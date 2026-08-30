#include "robot.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "secrets.h"

namespace {

const char* DANCES_DATASET = "pollen-robotics%2Freachy-mini-dances-library";

// 起動系は数秒かかるので長めに取る。
const uint16_t HTTP_TIMEOUT_MS = 8000;

int httpPost(const String& path, const String& body, String* out = nullptr) {
    HTTPClient http;
    if (!http.begin(String(ROBOT_HOST) + path)) {
        Serial.printf("[http] begin ng POST %s\n", path.c_str());
        return -1;
    }
    http.setTimeout(HTTP_TIMEOUT_MS);
    if (body.length()) http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);
    if (out && code > 0) *out = http.getString();
    http.end();
    Serial.printf("[http] POST %s -> %d\n", path.c_str(), code);
    return code;
}

String httpGet(const String& path) {
    HTTPClient http;
    if (!http.begin(String(ROBOT_HOST) + path)) {
        Serial.printf("[http] begin ng GET %s\n", path.c_str());
        return String();
    }
    http.setTimeout(HTTP_TIMEOUT_MS);
    int code = http.GET();
    String out;
    if (code == 200) out = http.getString();
    http.end();
    Serial.printf("[http] GET %s -> %d (%u bytes)\n", path.c_str(), code, out.length());
    return out;
}

bool daemonRunning() {
    String st = httpGet("/api/daemon/status");
    return st.indexOf("\"state\":\"running\"") >= 0;
}

}  // namespace

namespace robot {

bool ensureReady(String& detailOut) {
    if (!daemonRunning()) {
        // wake_up は必須パラメータ。無いと 422。false にして起床モーションを省く。
        if (httpPost("/api/daemon/start?wake_up=false", "") <= 0) {
            detailOut = "daemon start ng";
            return false;
        }
        bool up = false;
        for (int i = 0; i < 20; i++) {  // 最大10秒待つ
            delay(500);
            if (daemonRunning()) { up = true; break; }
        }
        if (!up) {
            detailOut = "daemon timeout";
            return false;
        }
    }
    // 本体を再起動するとトルクは disabled から始まるので毎回通す。
    Serial.println("[robot] enabling motors");
    if (httpPost("/api/motors/set_mode/enabled", "") != 200) {
        detailOut = "motors ng";
        return false;
    }
    detailOut = "ready";
    return true;
}

String playDance(const char* moveId) {
    Serial.printf("[robot] play %s\n", moveId);
    String body;
    String path = String("/api/move/play/recorded-move-dataset/") + DANCES_DATASET + "/" + moveId;
    if (httpPost(path, "", &body) != 200) return String();

    JsonDocument doc;
    if (deserializeJson(doc, body)) return String();
    const char* uuid = doc["uuid"];
    return uuid ? String(uuid) : String();
}

bool isPlaying() {
    String body = httpGet("/api/move/running");
    if (body.length() == 0) return false;  // 取れなかったときは止まった扱い
    JsonDocument doc;
    if (deserializeJson(doc, body)) return false;
    return doc.is<JsonArray>() && doc.as<JsonArray>().size() > 0;
}

bool stopMove(const String& uuid) {
    if (uuid.length() == 0) return false;
    String body = String("{\"uuid\":\"") + uuid + "\"}";
    return httpPost("/api/move/stop", body) == 200;
}

}  // namespace robot
