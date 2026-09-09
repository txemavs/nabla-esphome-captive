#include "nabla_captive.h"
#ifdef USE_NABLA_CAPTIVE
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/string_ref.h"
#include "esphome/components/wifi/scan_list.h"
#include "esphome/components/wifi/wifi_component.h"
#ifdef USE_PROVISIONING
#include "esphome/components/provisioning/provisioning.h"
#endif
#include "nabla_index.h"

namespace esphome::nabla_captive {

static const char *const TAG = "nabla_captive";

void NablaCaptive::handle_config(AsyncWebServerRequest *request) {
  AsyncResponseStream *stream = request->beginResponseStream(ESPHOME_F("application/json"));
  stream->addHeader(ESPHOME_F("cache-control"), ESPHOME_F("public, max-age=0, must-revalidate"));
  char mac_s[MAC_ADDRESS_PRETTY_BUFFER_SIZE];
  const char *mac_str = get_mac_address_pretty_into_buffer(mac_s);
#ifdef USE_ESP8266
  stream->print(ESPHOME_F("{\"mac\":\""));
  stream->print(mac_str);
  stream->print(ESPHOME_F("\",\"name\":\""));
  stream->print(App.get_name().c_str());
  stream->print(ESPHOME_F("\",\"aps\":[{}"));
#else
  stream->printf(R"({"mac":"%s","name":"%s","aps":[{})", mac_str, App.get_name().c_str());
#endif

  char escaped_ssid[32 * JSON_ESCAPE_MAX_EXPANSION + 1];
  {
    wifi::ScanResultsLock lock(wifi::global_wifi_component);
    const auto &results = wifi::global_wifi_component->get_scan_result();
    for (const auto &scan : results) {
      bool with_auth = false;
      if (!wifi::should_show_scan_entry(results, scan, with_auth))
        continue;

      json_escape_into_buffer(escaped_ssid, scan.get_ssid());
#ifdef USE_ESP8266
      stream->print(ESPHOME_F(",{\"ssid\":\""));
      stream->print(escaped_ssid);
      stream->print(ESPHOME_F("\",\"rssi\":"));
      stream->print(scan.get_rssi());
      stream->print(ESPHOME_F(",\"lock\":"));
      stream->print(with_auth);
      stream->print(ESPHOME_F("}"));
#else
      stream->printf(R"(,{"ssid":"%s","rssi":%d,"lock":%d})", escaped_ssid, scan.get_rssi(), with_auth);
#endif
    }
  }
  stream->print(ESPHOME_F("]}"));
  request->send(stream);
}

void NablaCaptive::handle_wifisave(AsyncWebServerRequest *request) {
  const auto &ssid = request->arg("ssid");
  const auto &psk = request->arg("psk");
  ESP_LOGI(TAG,
           "Requested WiFi Settings Change:\n"
           "  SSID='%s'\n"
           "  Password=" LOG_SECRET("'%s'"),
           ssid.c_str(), psk.c_str());
#ifdef USE_ESP8266
  wifi::global_wifi_component->save_wifi_sta(ssid.c_str(), psk.c_str());
#else
  this->defer([ssid, psk]() { wifi::global_wifi_component->save_wifi_sta(ssid.c_str(), psk.c_str()); });
#endif
  request->send(200, ESPHOME_F("text/plain"), ESPHOME_F("Saved. Connecting..."));
}

void NablaCaptive::setup() {
  this->disable_loop();
#ifdef USE_PROVISIONING
  if (provisioning::global_provisioning_manager != nullptr) {
    provisioning::global_provisioning_manager->add_on_closed_callback([this]() {
      if (this->active_) {
        ESP_LOGD(TAG, "Provisioning window closed; stopping nabla captive portal");
        this->end();
      }
    });
  }
#endif
}

void NablaCaptive::start() {
  this->base_->init();
  if (!this->initialized_) {
    this->base_->add_handler_without_auth(this);
  }

  network::IPAddress ip = wifi::global_wifi_component->wifi_soft_ap_ip();

#if defined(USE_ESP32)
  this->dns_server_ = make_unique<DNSServer>();
  this->dns_server_->start(ip);
#elif defined(USE_ARDUINO)
  this->dns_server_ = make_unique<DNSServer>();
  this->dns_server_->setErrorReplyCode(DNSReplyCode::NoError);
  this->dns_server_->start(53, ESPHOME_F("*"), ip);
#endif

  this->initialized_ = true;
  this->active_ = true;
  this->enable_loop();

  ESP_LOGV(TAG, "Nabla captive portal started");
}

void NablaCaptive::handleRequest(AsyncWebServerRequest *req) {
#ifdef USE_ESP32
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  StringRef url = req->url_to(url_buf);
#else
  const auto &url = req->url();
#endif
  if (url == ESPHOME_F("/config.json")) {
    this->handle_config(req);
    return;
  } else if (url == ESPHOME_F("/wifisave")) {
    this->handle_wifisave(req);
    return;
  }

#ifndef USE_ESP8266
  auto *response = req->beginResponse(200, ESPHOME_F("text/html"), NABLA_INDEX_GZ, sizeof(NABLA_INDEX_GZ));
#else
  auto *response = req->beginResponse_P(200, ESPHOME_F("text/html"), NABLA_INDEX_GZ, sizeof(NABLA_INDEX_GZ));
#endif
  response->addHeader(ESPHOME_F("Content-Encoding"), ESPHOME_F("gzip"));
  req->send(response);
}

NablaCaptive::NablaCaptive(web_server_base::WebServerBase *base) : base_(base) { global_nabla_captive = this; }
float NablaCaptive::get_setup_priority() const {
  return setup_priority::WIFI + 1.0f;
}
void NablaCaptive::dump_config() { ESP_LOGCONFIG(TAG, "Nabla Captive Portal:"); }

NablaCaptive *global_nabla_captive = nullptr;

}  // namespace esphome::nabla_captive

#endif
