#include "nabla_captive.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/wifi/wifi_component.h"
#include "nabla_index.h"

namespace esphome {
namespace nabla_captive {

static const char *const TAG = "nabla_captive";

void NablaCaptive::handle_config(AsyncWebServerRequest *request) {
  AsyncResponseStream *stream = request->beginResponseStream("application/json");
  stream->addHeader("cache-control", "public, max-age=0, must-revalidate");
  stream->printf(R"({"mac":"%s","name":"%s","aps":[{})", get_mac_address_pretty().c_str(), App.get_name().c_str());

  for (auto &scan : wifi::global_wifi_component->get_scan_result()) {
    if (scan.get_is_hidden())
      continue;

    stream->printf(R"(,{"ssid":"%s","rssi":%d,"lock":%d})", scan.get_ssid().c_str(), scan.get_rssi(),
                   scan.get_with_auth());
  }
  stream->print(F("]}"));
  request->send(stream);
}

void NablaCaptive::handle_wifisave(AsyncWebServerRequest *request) {
  std::string ssid = request->arg("ssid").c_str();
  std::string psk = request->arg("psk").c_str();
  ESP_LOGI(TAG, "Nabla Captive Portal Requested WiFi Settings Change:");
  ESP_LOGI(TAG, "  SSID='%s'", ssid.c_str());
  ESP_LOGI(TAG, "  Password=" LOG_SECRET("'%s'"), psk.c_str());
  wifi::global_wifi_component->save_wifi_sta(ssid, psk);
  wifi::global_wifi_component->start_scanning();
  request->redirect("/?save");
}

void NablaCaptive::setup() {}

void NablaCaptive::start() {
  this->base_->init();
  if (!this->initialized_) {
    this->base_->add_handler(this);
    this->base_->add_ota_handler();
  }

#ifdef USE_ARDUINO
  this->dns_server_ = make_unique<DNSServer>();
  this->dns_server_->setErrorReplyCode(DNSReplyCode::NoError);
  network::IPAddress ip = wifi::global_wifi_component->wifi_soft_ap_ip();
  this->dns_server_->start(53, "*", ip);
#endif

  this->base_->get_server()->onNotFound([this](AsyncWebServerRequest *req) {
    if (!this->active_ || req->host().c_str() == wifi::global_wifi_component->wifi_soft_ap_ip().str()) {
      req->send(404, "text/html", "File not found");
      return;
    }

    auto url = "http://" + wifi::global_wifi_component->wifi_soft_ap_ip().str();
    req->redirect(url.c_str());
  });

  this->initialized_ = true;
  this->active_ = true;
}

void NablaCaptive::handleRequest(AsyncWebServerRequest *req) {
  if (req->url() == "/") {
    auto *response = req->beginResponse_P(200, "text/html", NABLA_INDEX_GZ, sizeof(NABLA_INDEX_GZ));
    response->addHeader("Content-Encoding", "gzip");
    req->send(response);
    return;
  } else if (req->url() == "/config.json") {
    this->handle_config(req);
    return;
  } else if (req->url() == "/wifisave") {
    this->handle_wifisave(req);
    return;
  }
}

NablaCaptive::NablaCaptive(web_server_base::WebServerBase *base) : base_(base) { global_nabla_captive = this; }

float NablaCaptive::get_setup_priority() const {
  return setup_priority::WIFI + 1.0f;
}

void NablaCaptive::dump_config() { ESP_LOGCONFIG(TAG, "Nabla Captive Portal:"); }

NablaCaptive *global_nabla_captive = nullptr;

}  // namespace nabla_captive
}  // namespace esphome
