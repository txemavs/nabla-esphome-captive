import logging

import esphome.codegen as cg
from esphome.components import web_server_base, wifi
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
from esphome.config_helpers import filter_source_files_from_platform
import esphome.config_validation as cv
from esphome.const import (
    CONF_AP,
    CONF_ID,
    PLATFORM_BK72XX,
    PLATFORM_ESP32,
    PLATFORM_ESP8266,
    PLATFORM_LN882X,
    PLATFORM_RP2,
    PLATFORM_RTL87XX,
    PlatformFramework,
)
from esphome.core import CORE, coroutine_with_priority
from esphome.coroutine import CoroPriority
import esphome.final_validate as fv
from esphome.types import ConfigType

_LOGGER = logging.getLogger(__name__)


def AUTO_LOAD() -> list[str]:
    auto_load = ["web_server_base"]
    if CORE.is_esp32:
        auto_load.append("socket")
    return auto_load


DEPENDENCIES = ["wifi"]
CODEOWNERS = ["@txemavs"]

nabla_captive_ns = cg.esphome_ns.namespace("nabla_captive")
NablaCaptive = nabla_captive_ns.class_("NablaCaptive", cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(NablaCaptive),
            cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(
                web_server_base.WebServerBase
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on(
        [
            PLATFORM_ESP32,
            PLATFORM_ESP8266,
            PLATFORM_BK72XX,
            PLATFORM_LN882X,
            PLATFORM_RP2,
            PLATFORM_RTL87XX,
        ]
    ),
)


def _final_validate(config: ConfigType) -> None:
    full_config = fv.full_config.get()
    wifi_conf = full_config.get("wifi")

    if wifi_conf is None:
        raise cv.Invalid("Nabla captive portal requires the wifi component to be configured")

    if CONF_AP not in wifi_conf:
        _LOGGER.warning(
            "Nabla captive portal is enabled but no WiFi AP is configured. "
            "The captive portal will not be accessible. "
            "Add 'ap:' to your WiFi configuration to enable the captive portal."
        )

    from esphome.components import socket
    socket.consume_sockets(3, "nabla_captive")(config)
    socket.consume_sockets(1, "nabla_captive", socket.SocketType.UDP)(config)


FINAL_VALIDATE_SCHEMA = _final_validate


@coroutine_with_priority(CoroPriority.CAPTIVE_PORTAL)
async def to_code(config: ConfigType) -> None:
    paren = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])

    var = cg.new_Pvariable(config[CONF_ID], paren)
    await cg.register_component(var, config)
    cg.add_define("USE_NABLA_CAPTIVE")
    wifi.request_wifi_scan_results_lock()

    if CORE.using_arduino and (CORE.is_esp8266 or CORE.is_libretiny or CORE.is_rp2):
        cg.add_library("DNSServer", None)


FILTER_SOURCE_FILES = filter_source_files_from_platform(
    {
        "dns_server_esp32_idf.cpp": {
            PlatformFramework.ESP32_ARDUINO,
            PlatformFramework.ESP32_IDF,
        },
    }
)
