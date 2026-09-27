#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(beacon, LOG_LEVEL_DBG);

namespace {

constexpr k_timeout_t kReadInterval = K_SECONDS(5);

// This board (XIAO BLE Sense) has no onboard user button, so "status-gpio0"
// is a placeholder pointing at header pin D0 -- swap the pin in the board
// overlay once the real signal to monitor is known.
const struct gpio_dt_spec kStatusGpio = GPIO_DT_SPEC_GET(DT_ALIAS(status_gpio0), gpios);

// Requires a MAX17048 node aliased to "fuel-gauge0" in a board overlay — see
// boards/xiao_ble_nrf52840_sense.overlay.
const struct device *const kFuelGauge = DEVICE_DT_GET(DT_ALIAS(fuel_gauge0));

// 0xFFFF is reserved by the Bluetooth SIG for internal/test use only --
// replace with a real assigned Company Identifier before shipping.
constexpr uint8_t kTestCompanyId[2] = {0xFF, 0xFF};

uint8_t ReadBatteryPercent()
{
    if (!device_is_ready(kFuelGauge)) {
        LOG_ERR("Fuel gauge not ready");
        return 0;
    }

    union fuel_gauge_prop_val val;
    int err = fuel_gauge_get_prop(kFuelGauge, FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE, &val);
    if (err) {
        LOG_ERR("Fuel gauge read failed (err %d)", err);
        return 0;
    }

    LOG_DBG("Fuel gauge raw state of charge: %u%%", val.relative_state_of_charge);
    return val.relative_state_of_charge;
}

void BeaconThread(void *, void *, void *)
{
    LOG_INF("Beacon thread starting");

    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }
    LOG_DBG("Bluetooth enabled");

    if (!gpio_is_ready_dt(&kStatusGpio)) {
        LOG_ERR("Status GPIO not ready");
        return;
    }
    gpio_pin_configure_dt(&kStatusGpio, GPIO_INPUT);
    LOG_DBG("Status GPIO configured as input (port %s, pin %d)", kStatusGpio.port->name,
            kStatusGpio.pin);

    // [company_id_lo, company_id_hi, battery_percent, gpio_status]
    static uint8_t payload[4] = {kTestCompanyId[0], kTestCompanyId[1], 0, 0};
    const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, payload, sizeof(payload)),
    };

    err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    // This is a random static address generated fresh at boot (no identity
    // persisted via CONFIG_BT_SETTINGS), so it changes every power cycle.
    bt_addr_le_t addr = {0};
    size_t count = 1;
    bt_id_get(&addr, &count);
    LOG_INF("Beacon started, advertising as %s", bt_addr_le_str(&addr));

    while (true) {
        int gpio_status = gpio_pin_get_dt(&kStatusGpio);
        uint8_t battery_percent = ReadBatteryPercent();
        LOG_DBG("Raw reads: gpio_status=%d battery_percent=%u", gpio_status, battery_percent);

        payload[2] = battery_percent;
        payload[3] = static_cast<uint8_t>(gpio_status > 0);

        err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
        if (err) {
            LOG_ERR("Advertising update failed (err %d)", err);
        } else {
            LOG_INF("Beacon updated: battery=%u%% gpio=%d", battery_percent, gpio_status);
        }

        k_sleep(kReadInterval);
    }
}

} // namespace

K_THREAD_DEFINE(beacon_tid, 1024, BeaconThread, NULL, NULL, NULL, 7, 0, 0);
