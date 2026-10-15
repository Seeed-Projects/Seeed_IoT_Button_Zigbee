# Home Assistant with this Zigbee firmware

This page is the Home Assistant guide for the firmware in this repository (`Zigbee_Seeed_IoT_Button.merged.bin` and the Arduino sketch next to it).

Pairing still happens in ZHA or Zigbee2MQTT the usual way. After that, automations use the **two states** the coordinator shows.

The [Seeed product wiki](https://wiki.seeedstudio.com/seeed_iot_button_with_zigbee/) describes a later firmware with four Zigbee endpoints and virtual switches. If you flashed the merged binary from **this** repo, stay on this page.

## What Home Assistant shows

This firmware is a single Zigbee endpoint (endpoint 10) that reports an **IAS Zone** contact-switch status. ZHA typically adds one device named `Seeed Studio IoT_Button` with a two-state entity (often labeled **On** / **Off**, or **Turned on** / **Turned off** in the automation picker).

| Button action | What the firmware sends | What you select in Home Assistant |
| --- | --- | --- |
| Single click | IAS Zone status `0` (alarms clear) | **Off** / **Turned off** |
| Double click | IAS Zone status alarm bits set | **On** / **Turned on** |
| Long press (hold about 1 second) | Factory reset, then reboot | Pairing starts again; this is not an automation trigger |

That mapping is what users see after pairing this binary with ZHA: one device, two states, single click = off, double click = on.

## Confirm the mapping on your hub

1. In Home Assistant, open **Settings → Devices & Services → Zigbee Home Automation**.
2. Open the **Seeed Studio IoT_Button** device.
3. Leave **Developer tools → States** open, or watch the device page, and press the button.
4. A single click moves the entity to **off**. A double click moves it to **on**.
5. Copy the entity id from that page (for example `binary_sensor.seeed_studio_iot_button`). You will paste it into YAML below.

Zigbee2MQTT uses the same two-value IAS Zone report. After pairing, open the device and match the payload that appears on a single click to **off/clear**, and the payload on a double click to **on/alarm**.

## Create automations (ZHA)

### In the UI

1. Go to **Settings → Automations & Scenes → Create Automation**.
2. Add a **Device** trigger and choose **Seeed Studio IoT_Button**.
3. Pick **Turned off** for a single click, or **Turned on** for a double click.
4. Add the action (toggle a light, call a scene, and so on) and save.

Those two choices are the click mapping for this firmware.

### In YAML, from the entity state

Replace the entity ids with the ones on your device page.

Single click (state becomes `off`):

```yaml
alias: IoT Button single click
trigger:
  - platform: state
    entity_id: binary_sensor.seeed_studio_iot_button
    to: "off"
action:
  - service: light.toggle
    target:
      entity_id: light.living_room
```

Double click (state becomes `on`):

```yaml
alias: IoT Button double click
trigger:
  - platform: state
    entity_id: binary_sensor.seeed_studio_iot_button
    to: "on"
action:
  - service: scene.turn_on
    target:
      entity_id: scene.movie_time
```

Home Assistant runs these automations when the state **changes**. A second single click while the entity is already `off` keeps it `off`, so the single-click automation stays idle until a double click has set it `on` again. Use the two states for two different jobs (for example off → one scene, on → another), which matches how this firmware reports.

## Pairing recap

1. Put ZHA or Zigbee2MQTT into pairing mode.
2. Power the button and wait for it to join, or long-press about 1 second to factory-reset and join again.
3. The coordinator should discover **Seeed Studio IoT_Button**.
4. Then create automations from the two states above.

## Related product wiki

[Connect Seeed Studio IoT Button To Home Assistant via Zigbee](https://wiki.seeedstudio.com/seeed_iot_button_with_zigbee/) is the product-level walkthrough (coordinator setup, Arduino, later firmware features). Its automation step assumes the later multi-endpoint firmware (binary sensor plus virtual switches, with “Single press” / “Double press” style device actions). This repository’s merged binary follows the IAS Zone table on this page instead.
