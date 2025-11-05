import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, binary_sensor, text_sensor, sensor
from esphome.const import CONF_ID, CONF_UART_ID, UNIT_EMPTY, ICON_PULSE

AUTO_LOAD = ["binary_sensor", "text_sensor", "sensor", "uart"]

mr24fdb1_ns = cg.esphome_ns.namespace("mr24fdb1")
MR24FDB1 = mr24fdb1_ns.class_("MR24FDB1", cg.Component, uart.UARTDevice)

CONF_PRESENCE = "presence"
CONF_FALL = "fall"
CONF_STATE = "state"
CONF_ENV_TRIPLET = "environment_triplet"
CONF_ENV_LABEL   = "environment_label"
CONF_APPROACH    = "approach_state"
CONF_FALL_STATE  = "fall_state"
CONF_SIGN_PARAM  = "sign_parameter"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MR24FDB1),
            cv.Optional(CONF_PRESENCE): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_FALL): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_STATE): text_sensor.text_sensor_schema(),
            cv.Optional(CONF_ENV_TRIPLET): text_sensor.text_sensor_schema(),
            cv.Optional(CONF_ENV_LABEL): text_sensor.text_sensor_schema(),
            cv.Optional(CONF_APPROACH): text_sensor.text_sensor_schema(),
            cv.Optional(CONF_FALL_STATE): text_sensor.text_sensor_schema(),
            cv.Optional(CONF_SIGN_PARAM): sensor.sensor_schema(unit_of_measurement=UNIT_EMPTY, icon=ICON_PULSE),
        }
    ).extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await uart.register_uart_device(var, config)
    await cg.register_component(var, config)

    if CONF_PRESENCE in config:
        pres = await binary_sensor.new_binary_sensor(config[CONF_PRESENCE])
        cg.add(var.set_presence_sensor(pres))

    if CONF_FALL in config:
        fall = await binary_sensor.new_binary_sensor(config[CONF_FALL])
        cg.add(var.set_fall_sensor(fall))

    if CONF_STATE in config:
        st = await text_sensor.new_text_sensor(config[CONF_STATE])
        cg.add(var.set_state_text_sensor(st))

    if CONF_ENV_TRIPLET in config:
        t = await text_sensor.new_text_sensor(config[CONF_ENV_TRIPLET])
        cg.add(var.set_env_triplet_text_sensor(t))

    if CONF_ENV_LABEL in config:
        t = await text_sensor.new_text_sensor(config[CONF_ENV_LABEL])
        cg.add(var.set_env_label_text_sensor(t))

    if CONF_APPROACH in config:
        t = await text_sensor.new_text_sensor(config[CONF_APPROACH])
        cg.add(var.set_approach_text_sensor(t))

    if CONF_FALL_STATE in config:
        t = await text_sensor.new_text_sensor(config[CONF_FALL_STATE])
        cg.add(var.set_fall_state_text_sensor(t))

    if CONF_SIGN_PARAM in config:
        s = await sensor.new_sensor(config[CONF_SIGN_PARAM])
        cg.add(var.set_sign_parameter_sensor(s))
