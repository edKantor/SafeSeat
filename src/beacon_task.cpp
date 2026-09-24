#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/bluetooth/bluetooth.h>

namespace {

constexpr k_timeout_t kReadInterval = K_SECONDS(5);

// "sw0" is this board's separate user button (P1.02), not the physical reset
// button. Swap the alias/pin here once the real signal to monitor is known.
const struct gpio_dt_spec kStatusGpio = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

// Requires a MAX17048 node aliased to "fuel-gauge0" in a board overlay — see
// boards/adafruit_feather_nrf52840_nrf52840.overlay.
const struct device *const kFuelGauge = DEVICE_DT_GET(DT_ALIAS(fuel_gauge0));

// 0xFFFF is reserved by the Bluetooth SIG for internal/test use only --
// replace with a real assigned Company Identifier before shipping.
constexpr uint8_t kTestCompanyId[2] = {0xFF, 0xFF};

uint8_t ReadBatteryPercent()
{
    if (!device_is_ready(kFuelGauge)) {
        printk("Fuel gauge not ready\n");
        return 0;
    }

    union fuel_gauge_prop_val val;
    int err = fuel_gauge_get_prop(kFuelGauge, FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE, &val);
    if (err) {
        printk("Fuel gauge read failed (err %d)\n", err);
        return 0;
    }

    return val.relative_state_of_charge;
}

void BeaconThread(void *, void *, void *)
{
    int err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    if (!gpio_is_ready_dt(&kStatusGpio)) {
        printk("Status GPIO not ready\n");
        return;
    }
    gpio_pin_configure_dt(&kStatusGpio, GPIO_INPUT);

    // [company_id_lo, company_id_hi, battery_percent, gpio_status]
    static uint8_t payload[4] = {kTestCompanyId[0], kTestCompanyId[1], 0, 0};
    const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, payload, sizeof(payload)),
    };

    err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    while (true) {
        int gpio_status = gpio_pin_get_dt(&kStatusGpio);
        uint8_t battery_percent = ReadBatteryPercent();

        payload[2] = battery_percent;
        payload[3] = static_cast<uint8_t>(gpio_status > 0);

        err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
        if (err) {
            printk("Advertising update failed (err %d)\n", err);
        } else {
            printk("Beacon updated: battery=%u%% gpio=%d\n", battery_percent, gpio_status);
        }

        k_sleep(kReadInterval);
    }
}

} // namespace

K_THREAD_DEFINE(beacon_tid, 1024, BeaconThread, NULL, NULL, NULL, 7, 0, 0);
