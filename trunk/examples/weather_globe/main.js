//
// Copyright 2007 Google Inc.
// All Rights Reserved.
//


// Timers
var _OpenCloseToken = 0;
var _AnimationTimer = 0;
var _AnimationTimer1 = 0;
var _ChanceTimer = 0;
var _ShakeTimer = 0;

var kOpacityLimit = 200;
var kMaxSnowWidth = 11;

var kViewMinHeight = 150;
var kViewMinWidth = 200;
var kViewMaxHeight = 275;
var kViewMaxWidth = 200;

var kShakeLimit = 1;

var kWeatherRefreshTime = 15 * /* 60 * */ 1000;

var randomNumbers = false;
var kRandomNumbersLength = 25;
var random_i = 0;

var randomNumbers3 = false;
var kRandomNumbers3Length = 50;
var random3_i = 0;

var randomNumbers6 = false;
var kRandomNumbers6Length = 25;
var random6_i = 0;

var randomOpacities = false;
var kRandomOpacitiesLength = 30;
var randomo_i = 0;

var mouseOverCount = 0;

var kCurrentCityMaxWidth_ = 172;
var kCurrentCityMinWidth_ = 95;
var kOpenCloseWidth_ = 18;
var kNameplateLeftWidth_ = 8;

function _Window_OnOpen() {
  Weather_Init();
  setInterval(Weather_Init, kWeatherRefreshTime);

  if (options.getValue("TrayOpen")) {
    OpenClose.src = "buttons/b_close_u.png";
  } else {
    OpenClose.src = "buttons/b_open_u.png";
  }
  DoOpenClose();

  var lang = undefined;
  try {
    lang = system.languageCode();
  } catch (e) {
    lang = strings.DEFAULT_LANGUAGE;
  }
  options.putValue("language", lang);
}

//
// NAMEPLATE 'CHECKBOX'
//
function nameplate_OnMouseOver() {
  if (options.getValue("TrayOpen"))
    OpenClose.src = "buttons/b_close_h.png";
  else
    OpenClose.src = "buttons/b_open_h.png";
}

function nameplate_OnMouseOut() {
  if (options.getValue("TrayOpen"))
    OpenClose.src = "buttons/b_close_u.png";
  else
    OpenClose.src = "buttons/b_open_u.png";
}

function nameplate_OnMouseDown() {
  if (options.getValue("TrayOpen"))
    OpenClose.src = "buttons/b_close_d.png";
  else
    OpenClose.src = "buttons/b_open_d.png";
}

function nameplate_OnMouseUp() {
  if (options.getValue("TrayOpen"))
    OpenClose.src = "buttons/b_open_h.png";
  else
    OpenClose.src = "buttons/b_close_h.png";
  options.putValue("TrayOpen", !options.getValue("TrayOpen"));
}

function currentcity_OnSize() {
  var width = current_city.offsetWidth;
  if (width > kCurrentCityMaxWidth_) {
    width = kCurrentCityMaxWidth_;
  } else if (width < kCurrentCityMinWidth_) {
    width = kCurrentCityMinWidth_;
  }

  var deltaX = (width - nameplate_background.width) / 2;
  nameplate.x = nameplate.x - deltaX;
  nameplate.width = width + kOpenCloseWidth_ + kNameplateLeftWidth_;

  nameplate_left.x = nameplate_left.x - deltaX;

  nameplate_background.width = width;

  if (current_city.offsetWidth < width) {
    current_city.x = ((width - current_city.offsetWidth) / 2) +
                     kNameplateLeftWidth_;
  } else {
    current_city.x = 0;
  }
  OpenClose.x = nameplate_background.width + kNameplateLeftWidth_;
}

//
// ANIMATE 'SHAKE' USING MOUSEOVERS
//
function Globe_OnMouseOver() {
  if (mouseOverCount >= kShakeLimit) {
    return;
  }
  mouseOverCount++;
  setTimeout(StartFadeOut, 3550);
  // Uncomment this to test continuously memory leaks.
  // setTimeout(Globe_OnMouseOver, 5000);
  if (Weather_currentWeatherType != WeatherType.Rain &&
      Weather_currentWeatherType != WeatherType.Storm &&
      Weather_currentWeatherType != WeatherType.Thunderstorm &&
      Weather_currentWeatherType != WeatherType.Snow &&
      Weather_currentWeatherType != WeatherType.Flurries) {
    resetAnimation("sleet", true);
    _ShakeTimer = beginAnimation(Fade, 0, kOpacityLimit, 500);
  }
  _AnimationTimer = beginAnimation("_AnimateCallback(true, true)", 0, 50, 4000);
}

// Gracefully end the snowfall
function StartFadeOut() {
  if (--mouseOverCount == 0) {
    if (Weather_currentWeatherType != WeatherType.Rain &&
        Weather_currentWeatherType != WeatherType.Storm &&
        Weather_currentWeatherType != WeatherType.Thunderstorm &&
        Weather_currentWeatherType != WeatherType.Snow &&
        Weather_currentWeatherType != WeatherType.Flurries) {
      beginAnimation(Fade, kOpacityLimit, 0, 500);
    }
  }
}

function Fade() {
  animated.opacity = event.value;
  animated1.opacity = event.value;
}

//
// ANIMATE FORECAST TRAY
//
function _Window_OnOptionChanged() {
  if (event.propertyName == "TrayOpen") {
    DoOpenClose();
  }
}

function DoOpenClose() {
  var Target;

  if (options.getValue("TrayOpen")) {
    Window_Shrink();
    Target = 255;
  } else {
    Target = 0;
  }

  var Distance = Math.abs (forecast.opacity - Target);
  var TimeToAnimate = Distance * 2.5;

  if (_OpenCloseToken != 0) {
    cancelAnimation(_OpenCloseToken);
    _OpenCloseToken = 0;
  }

  if (!options.getValue("TrayOpen")) {
    setTimeout(Window_Shrink, TimeToAnimate);
  }
  _OpenCloseToken = beginAnimation(OnOpenCloseAnim, forecast.opacity, Target, TimeToAnimate);
}

// Resize window to fit tightly without the globe
function Window_Shrink() {
  if (!options.getValue("TrayOpen")) {
    view.width = kViewMinWidth;
    view.height = kViewMinHeight;
  } else {
    view.width = kViewMaxWidth;
    view.height = kViewMaxHeight;
  }
}

function OnOpenCloseAnim() {
  forecast.opacity = event.value;
}

//
// ANIMATE RAIN, SNOW, FLURRIES, ETC
//
function UpdateAnimation() {
  clearInterval(_AnimationTimer);
  switch (Weather_currentWeatherType) {
    case WeatherType.Flurries:
      resetAnimation("sleet", false);
      current_weather.src = "large/cloud.png";
      _AnimationTimer = setInterval("_AnimateCallback(false, false)", 175);
      break;
    case WeatherType.Snow:
      resetAnimation("snow", false);
      current_weather.src = "large/cloud.png";
      _AnimationTimer = setInterval("_AnimateCallback(false, false)", 150);
      break;
    case WeatherType.Rain:
      resetAnimation("rain", false);
      current_weather.src = "large/darkercloud.png";
      _AnimationTimer = setInterval("_AnimateCallback(false, true)", 175);
      break;
    case WeatherType.Thunderstorm:
      resetAnimation("rain", true);
      current_weather.src = "large/darkcloud_thunder.png";
      _AnimationTimer = setInterval("_AnimateCallback(true, true)", 125);
      break;
    case WeatherType.Storm:
      resetAnimation("rain", true);
      current_weather.src = "large/darkercloud.png";
      _AnimationTimer = setInterval("_AnimateCallback(true, true)", 125);
      break;
    default:
      animated.opacity = 0;
      animated1.opacity = 0;
      break;
  }
}

function resetAnimation(icon, isHeavy) {
  var e = new Enumerator(animated.children);
  for (; !e.atEnd(); e.moveNext()) {
    e.item().src = "animations/" + icon + GetRandomAnim() + ".png";
  }
  animated.opacity = 225;
  if (isHeavy) {
    var e = new Enumerator(animated1.children);
    for (; !e.atEnd(); e.moveNext()) {
      e.item().src = "animations/" + icon + GetRandomAnim() + ".png";
    }
    animated1.opacity = 225;
  } else {
    animated1.opacity = 0;
  }
}

function _AnimateCallback(isHeavy, isFast) {
  Animate(animated, isFast);
  if (isHeavy) {
    Animate(animated1, isFast);
  }
}

function Animate(element, isFast) {
  var e = new Enumerator(element.children);
  for (; !e.atEnd(); e.moveNext()) {
    var x = e.item().x + GetRandomXOffset();
    var y;
    y = e.item().y + GetRandomYOffset(isFast);


    x = x % (element.width - kMaxSnowWidth);
    if (x > 0 && x < element.width - kMaxSnowWidth) {
      e.item().x = x;
    } else {
      e.item().x = GetRandomDelta(kMaxSnowWidth, element.width - kMaxSnowWidth);
    }
    e.item().y = Math.abs(y % (element.height - kMaxSnowWidth));

    var opacity = e.item().opacity + GetRandomOpacity();
    if (opacity >= 175 && opacity <= 255) {
      e.item().opacity = opacity;
    }
  }

}

function GetRandomDelta(min, max) {
  // Generate a set of numbers once
  if (!randomNumbers) {
    randomNumbers = new Array(kRandomNumbersLength);
    for (var i = 0; i < kRandomNumbersLength; ++i) {
      randomNumbers[i] = Math.random();
    }
  }
  random_i = (random_i + 1) % kRandomNumbersLength;
  var delta = Math.round((randomNumbers[random_i] * (max - min)) / 1.0);
  return delta + min;
}

function SeedRandomNumbers(min, max, length) {
  var ret = new Array();
  for (var i = 0; i < length; ++i) {
    var delta = Math.round((Math.random() * (max - min)) / 1.0);
    ret[i] = delta + min;
  }
  return ret;
}

function GetRandomXOffset() {
  if (!randomNumbers3) {
    randomNumbers3 = SeedRandomNumbers(0, 2, kRandomNumbers3Length);
  }
  random3_i = (random3_i + 1) % kRandomNumbers3Length;
  return randomNumbers3[random3_i] - 1;
}

function GetRandomYOffset(isFast) {
  if (isFast) {
    if (!randomNumbers6) {
      randomNumbers6 = SeedRandomNumbers(0, 5, kRandomNumbers3Length);
    }
    random6_i = (random6_i + 1) % kRandomNumbers6Length;
    return randomNumbers6[random6_i];
  } else {
    if (!randomNumbers3) {
      randomNumbers3 = SeedRandomNumbers(0, 2, kRandomNumbers3Length);
    }
    random3_i = (random3_i + 1) % kRandomNumbers3Length;
    return randomNumbers3[random3_i];
  }
}

function GetRandomAnim() {
  if (!randomNumbers3) {
    randomNumbers3 = SeedRandomNumbers(0, 2, kRandomNumbers3Length);
  }
  random3_i = (random3_i + 1) % kRandomNumbers3Length;
  return randomNumbers3[random3_i] + 1;
}

function GetRandomOpacity() {
  if (!randomOpacities) {
    randomOpacities = SeedRandomNumbers(-30, 30, kRandomOpacitiesLength);
  }
  randomo_i = (randomo_i + 1) % kRandomOpacitiesLength;
  return randomOpacities[randomo_i];
}

//
// ANIMATE CHANCEOF QUESTION MARK
//
function StartChance() {
  chance.opacity = 50;
  _ChanceTimer = setInterval(Pulsate, 100);
}

function StopChance() {
  chance.opacity = 0;
  clearInterval(_ChanceTimer);
}

var pulsate_sign = 1;

function Pulsate() {
  var curr = chance.opacity + pulsate_sign;
  if (curr > 255) {
    pulsate_sign = -3;
  } else if (curr < 50) {
    pulsate_sign = 3;
  } else {
    chance.opacity = curr;
  }
}
