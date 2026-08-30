#pragma once
#include <Arduino.h>

// Reachy Mini の daemon REST を叩く薄いラッパ。
// 認証は無い。ラジアン単位。データセット名の / は %2F にエンコード済みの定数を使う。
namespace robot {

// 起動シーケンス。backend が stopped だと 503、モーターが disabled だと
// uuid が返るのに動かないので、両方まとめて面倒を見る。冪等。
bool ensureReady(String& detailOut);

// ダンスを再生する。成功したら uuid、失敗したら空文字列。非ブロッキング。
String playDance(const char* moveId);

// 再生中かどうか。GET /api/move/running が [] ならアイドル。
bool isPlaying();

// 再生を止める。uuid が必須。
bool stopMove(const String& uuid);

}  // namespace robot
