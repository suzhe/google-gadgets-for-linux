//
// Copyright 2007 Google Inc.
// All Rights Reserved.
//
/**
 * @fileoverview Contains parsing code for the iGoogle weather feed. Assumes
 * only one city is requested.
 */

var kWeatherUrl = "ig/api?hl=";
var kCityRegionSeparator = ", ";
var kWeatherQueryParam = "&weather=";
var kHostQueryParam = "&host="
var kOnlyLatLongParamPrefix = ",,,";
var kCountriesUrl = "ig/countries?output=xml&hl=";
var kCitiesUrl = "ig/cities?output=xml&hl=";
var kCountryParam = "&country=";
var kIsoLatLongSeparator = ",";
var kGoogleURI = "http://www.google.com/";

/**
 * Enum for amount of animation required
 * @enum{Number}
 */
var WeatherAnimationType = { None: 1, Light: 2, Medium: 3, Heavy: 4 };

/**
 * Map of weather conditions to icon used depending on time of day
 */
var WeatherType = {
    MostlyCloudy : { day: "mostlycloudy_day"   , night: "mostlycloudy_night"   },
    Cloudy       : { day: "cloudy_day"         , night: "cloudy_night"         },
    MostlySunny  : { day: "mostlysunny_day"    , night: "mostlysunny_night"    },
    Sunny        : { day: "sunny_day"          , night: "sunny_night"          },
    Fog          : { day: "fog"                , night: "fog"                  },
    Haze         : { day: "haze"               , night: "haze"                 },
    Icy          : { day: "icy"                , night: "icy"                  },

    Flurries     : { day: "flurries"           , night: "flurries"             },
    Snow         : { day: "snow"               , night: "snow"                 },
    Rain         : { day: "rain"               , night: "rain"                 },
    Thunderstorm : { day: "thunderstorm"       , night: "thunderstorm"         },
    Storm        : { day: "storm"              , night: "storm"                },

    ChanceOfSnow : { day: "chanceofsnow_day"   , night: "chanceofsnow_night"   },
    ChanceOfRain : { day: "chanceofrain_day"   , night: "chanceofrain_night"   },
    ChanceOfSleet: { day: "chanceofsleet_day"  , night: "chanceofsleet_night"  },
    ChanceOfStorm: { day: "chanceofstorm_day"  , night: "chanceofstorm_night"  },
    ChanceOfTStorm:{ day: "chanceoftstorm_day" , night: "chanceoftstorm_night" },

    Error        : { day: "haze"               , night: "haze"                 }
}

/**
 * Set by Weather_GetIcon for today's condition
 * @type WeatherType
 */
var Weather_currentWeatherType = WeatherType.Cloudy;

/**
 * Latest valid parsed data
 * @type WeatherData
 */
var Weather_currentWeatherData = undefined;

/**
 * Class to contain parsed weather data. Everything stored as strings except
 * date (as Date object).
 * @constructor
 */
function WeatherData() {
  this.cityName = undefined;
  this.date = undefined;

  this.currentTemp = undefined;
  this.currentIcon = undefined;
  this.currentCondition = undefined;

  // Arrays of forecast information. Starts with current day's information
  this.forecastHighs = new Array();
  this.forecastLows = new Array();
  this.forecastDays = new Array();
  this.forecastConditions = new Array();
  this.forecastIcons = new Array();
}

/**
 * Creates and runs query to get weather data, and handles callback.
 */
function Weather_Init() {
  var request = new XMLHttpRequest();
  request.onreadystatechange = Weather_OnReceivedData;
  request.open("GET", Weather_BuildWeatherURL(), true);
  request.send();

  function Weather_OnReceivedData() {
    if (request && request.readyState == 4 && request.status == 200) {
      error.opacity = 0;
      Weather_ParseData(request.responseXML);
    }
  }
}

/**
 * Updates display to indicate error.
 */
function Weather_DisplayError() {
  current_weather.src = Weather_GetWeatherIcon("error", true);
  UpdateAnimation();
  StartChance();
  current_city.innerText = options.getValue(kCityName);

  error.opacity = 255;

  high_temp.innerText = "";
  low_temp.innerText = "";
  current_temp.innerText = "";

  for (var index = 0; index < 4; ++index) {
    forecast.children.item("high_temp_" + index).innerText = "";
    forecast.children.item("low_temp_" + index).innerText = "";
    forecast.children.item("date_" + index).innerText = "";
    forecast.children.item("weather_" + index).src = "question.png";
    forecast.children.item("weather_" + index).tooltip = "";
  }
}

/**
 * Parses an XML response from iGoogle, and stores it in Weather_currentWeatherData.
 * If parsing is successful, display is updated with the new data, otherwise an
 * error message is displayed. Assumes only one city is requested.
 * @param {String} responseXML XML response from iGoogle.
 */
function Weather_ParseData(responseXML) {
  var newWeatherData = new WeatherData();

  try {
    var cities = responseXML.getElementsByTagName("weather");
    var currentCity = cities[0];
    var cityInfo = currentCity.getElementsByTagName("forecast_information")[0];

    newWeatherData.cityName = options.getValue(kCityName);

    // which scale forecast temperatures are given
    var is_metric = IsMetric(cityInfo);

    // Decode the current date of the feed.
    var forecast_date = GetData(cityInfo, "forecast_date").split(/-/);
    newWeatherData.date = new Date(forecast_date[0],
                                   forecast_date[1] - 1,
                                   forecast_date[2]);

    var currentConditions =
        currentCity.getElementsByTagName("current_conditions")[0];

    if (options.getValue(kUseMetric)) {
      newWeatherData.currentTemp = GetData(currentConditions, "temp_c") +
                                   String.fromCharCode(0xB0) + "C";
    } else {
      newWeatherData.currentTemp = GetData(currentConditions, "temp_f") +
                                   String.fromCharCode(0xB0) + "F";
    }

    var current_icon = GetData(currentConditions, "icon");
    newWeatherData.currentIcon = Weather_GetWeatherIcon(current_icon, true);
    newWeatherData.currentCondition = GetData(currentConditions, "condition");

    // update the forecast pane
    var forecasts = currentCity.getElementsByTagName("forecast_conditions");
    for (var i = 0; i < forecasts.length; i++) {
      var curr = forecasts[i];

      // calculate element index from the given forecast day
      var day = GetData(curr, "day_of_week");

      var high = "H " + Weather_GetTemperature(curr, "high", is_metric);
      var low = "L " + Weather_GetTemperature(curr, "low", is_metric);

      newWeatherData.forecastHighs[i] = high;
      newWeatherData.forecastLows[i] = low;
      newWeatherData.forecastDays[i] = day;

      var icon = GetData(curr, "icon");
      newWeatherData.forecastIcons[i] = Weather_GetWeatherIcon(icon, false, day);
      newWeatherData.forecastConditions[i] = GetData(curr, "condition");
    }

    // succesfully parsed data.. update display and save the data
    Weather_currentWeatherData = newWeatherData;
    Weather_UpdateDisplay(Weather_currentWeatherData);

  } catch (e) {
    gadget.debug.error("weather_parsedata: " + e);
    Weather_DisplayError();
  }
}

/**
 * Updates the display for the given weatherData. Starts animations depending
 * on the current weather type.
 * @param {WeatherData} weatherData Data used to update the display.
 */
function Weather_UpdateDisplay(weatherData) {
  // update globe
  current_city.innerText = weatherData.cityName;
  current_temp.innerText = weatherData.currentTemp;
  current_weather.src = weatherData.currentIcon;
  current_weather.tooltip = weatherData.currentCondition;

  high_temp.innerText = weatherData.forecastHighs[0];
  low_temp.innerText = weatherData.forecastLows[0];

  var type = Weather_currentWeatherType;
  if (type == WeatherType.ChanceOfSleet ||
      type == WeatherType.ChanceOfSnow ||
      type == WeatherType.ChanceOfRain ||
      type == WeatherType.ChanceOfStorm ||
      type == WeatherType.ChanceOfTStorm) {
    StartChance();
  } else {
    StopChance();
  }

  UpdateAnimation();

  // update forecast pane
  for (var i = 0; i < weatherData.forecastDays.length; ++i) {
    forecast.children.item("high_temp_" + i).innerText =
        weatherData.forecastHighs[i];
    forecast.children.item("low_temp_" + i).innerText =
        weatherData.forecastLows[i];
    forecast.children.item("date_" + i).innerText =
        weatherData.forecastDays[i];
    forecast.children.item("weather_" + i).src =
        weatherData.forecastIcons[i];
    forecast.children.item("weather_" + i).tooltip =
        weatherData.forecastConditions[i];
  }
}

/**
 * Translates the weather icon from the xml element to the right icon to use,
 * depending on where the icon is to be used, and the current day. Large icons
 * are returned for the globe, and mini ones for the forecast pane. Day icons
 * are always returned except if it is nighttime for the current day. Handles
 * tranaslation for new Japanese icons.
 * @param {XMLElement} condition XMLNode containing the condition data.
 * @param {Boolean} isMain True if used in the globe, False if used in the
 *     forecast pane.
 * @param {String} day Day the icon is to be used for.
 */
function Weather_GetWeatherIcon(condition, isMain, opt_day) {
  var pattern = /.*\/([\w]+).gif$/i;
  var match = condition.match(pattern);
  if (match) {
    condition = match[1];
  }

  var type;
  switch (condition.toLowerCase()) {
    case "error":
      type = WeatherType.Error; break;
    case "mostly_cloudy":
      type = WeatherType.MostlyCloudy; break;
    case "cloudy":
      type = WeatherType.Cloudy; break;
    case "mostly_sunny":
      type = WeatherType.MostlySunny; break;
    case "sunny":
      type = WeatherType.Sunny; break;
    case "flurries":
      type = WeatherType.Flurries; break;
    case "fog":
      type = WeatherType.Fog; break;
    case "haze":
      type = WeatherType.Haze; break;
    case "icy":
      type = WeatherType.Icy; break;
    case "chance_of_sleet":
      type = WeatherType.ChanceOfSleet; break;
    case "snow":
      type = WeatherType.Snow; break;
    case "chance_of_snow":
      type = WeatherType.ChanceOfSnow; break;
    case "rain":
      type = WeatherType.Rain; break;
    case "chance_of_rain":
      type = WeatherType.ChanceOfRain; break;
    case "thunderstorm":
      type = WeatherType.Thunderstorm; break;
    case "chance_of_tstorm":
      type = WeatherType.ChanceOfTStorm; break;
    case "storm":
      type = WeatherType.Storm; break;
    case "chance_of_storm":
      type = WeatherType.ChanceOfStorm; break;
    // special case for new japanese weather icons
    case "jp_sunny":
      type = WeatherType.Sunny; break;
    case "jp_sunnysometimescloudy":
      type = WeatherType.MostlySunny; break;
    case "jp_sunnysometimesrainy":
      type = WeatherType.ChanceOfRain; break;
    case "jp_sunnysometimessnowy":
      type = WeatherType.ChanceOfSnow; break;
    case "jp_sunnythencloudy":
      type = WeatherType.MostlySunny; break;
    case "jp_sunnythenrainy":
      type = WeatherType.ChanceOfRain; break;
    case "jp_sunnythensnowy":
      type = WeatherType.ChanceOfSnow; break;
    case "jp_cloudy":
      type = WeatherType.Cloudy; break;
    case "jp_cloudysometimessunny":
      type = WeatherType.MostlyCloudy; break;
    case "jp_cloudysometimesrainy":
      type = WeatherType.ChanceOfRain; break;
    case "jp_cloudysometimessnowy":
      type = WeatherType.ChanceOfSnow; break;
    case "jp_cloudythensunny":
      type = WeatherType.MostlyCloudy; break;
    case "jp_cloudythenrainy":
      type = WeatherType.ChanceOfRain; break;
    case "jp_cloudythensnowy":
      type = WeatherType.ChanceOfSnow; break;
    case "jp_rainy":
      type = WeatherType.Rain; break;
    case "jp_rainysometimessunny":
      type = WeatherType.ChanceOfRain; break;
    case "jp_rainysometimescloudy":
      type = WeatherType.ChanceOfRain; break;
    case "jp_rainysometimessnowy":
      type = WeatherType.Rain; break;
    case "jp_rainythensunny":
      type = WeatherType.ChanceOfRain; break;
    case "jp_rainythencloudy":
      type = WeatherType.ChanceOfRain; break;
    case "jp_rainythensnowy":
      type = WeatherType.Rain; break;
    case "jp_snowy":
      type = WeatherType.Snow; break;
    case "jp_snowysometimessunny":
      type = WeatherType.ChanceOfSnow; break;
    case "jp_snowysometimescloudy":
      type = WeatherType.ChanceOfSnow; break;
    case "jp_snowysometimesrainy":
      type = WeatherType.Snow; break;
    case "jp_snowythensunny":
      type = WeatherType.ChanceOfSnow; break;
    case "jp_snowythencloudy":
      type = WeatherType.ChanceOfSnow; break;
    case "jp_snowythenrainy":
      type = WeatherType.Snow; break;
    default:
      type = WeatherType.MostlyCloudy; break;
  }
  var icon;
  if (isMain) {
    Weather_currentWeatherType = type;
    // Use special icons if it's an animated weather type
    if (type == WeatherType.Flurries || type == WeatherType.Snow) {
      icon = "large/cloud.png";
    } else if (type == WeatherType.Rain || type == WeatherType.Storm) {
      icon = "large/darkercloud.png";
    } else if (type == WeatherType.Thunderstorm) {
      icon = "large/darkcloud_thunder.png";
      // Use night or day icon otherwise
    } else if (IsDaytime()) {
      icon = "large/" + type.day + ".png";
    } else {
      icon = "large/" + type.night + ".png";
    }
  } else { // Mini icons
    if (opt_day == "Today" && !IsDaytime()) {
      // REMOVE_GADGET_BEGIN
      // TODO(V6:cyin): i18n this
      // REMOVE_GADGET_END
      icon = "mini/mini_" + type.night + ".png";
    } else {
      icon = "mini/mini_" + type.day + ".png";
    }
  }
  gadget.debug.trace("Condition: " + condition + "  Icon: "+icon);
  return icon;
}

/**
 * Creates the query string to fetch the weather data for the current city.
 */
function Weather_BuildWeatherURL() {
  var languageCode = options.getValue("language");
  var url = kGoogleURI + kWeatherUrl + languageCode;
  if (languageCode == 'ja') {
    url += kHostQueryParam + "www.google.co.jp";
  }
  url += kWeatherQueryParam;

  switch(options.getValue(kLocationSetting)) {
    case kZipcode:
      url += options.getValue(kZipcode);
      break;
    case kLatLong:
      url += kOnlyLatLongParamPrefix + options.getValue(kLatLong);
      break;
    case kCityName:
      url += options.getValue(kCityName);
      break;
    default:
      gadget.debug.error("Invalid location setting = "+options.getValue(kLocationSetting));
      break;
  }
  gadget.debug.trace("URL: " + url);
  return url;
}

/**
 * Returns temperature in the given unit from the element.
 * @param {XMLElement} element Element containing the temperature data
 * @param {String} tag_name Name of the temperature property in the element
 * @param {Boolean} is_metric True if temperature should be returned in Celsius
 * @return {Number} Converted temperature
 */
function Weather_GetTemperature(element, tag_name, is_metric) {
  var temp = GetData(element, tag_name);
  if (options.getValue(kUseMetric)) {
    if (!is_metric) {
      temp = F_to_C(temp);
    }
    return temp + String.fromCharCode(0xB0) + "C";

  } else {
    if (is_metric) {
      temp = C_to_F(temp);
    }
    return temp + String.fromCharCode(0xB0) + "F";
  }
}

/**
 * Helper function to retrieve data from an XML element. This should be used in
 * a try/catch.
 * @param {XMLNode} node The XML node
 * @param {String} element Name of the property we want to extract
 * @return {String} Extracted data.
 */
function GetData(node, element) {
  return node.getElementsByTagName(element)[0].getAttribute("data");
}

/**
 * Helper function to determine if it's daytime (between 6am and 6pm) for the
 * user.
 * @return {Boolean} True if it's daytime
 */
function IsDaytime() {
  var day = new Date;
  var hour = day.getHours();
  return (hour >= 6 && hour <= 18);
}

/**
 * Converts from Fahrenheit to Celsius.
 */
function F_to_C(temperature) {
  return Math.round((parseInt(temperature) - 32) * (5.0/9.0));
}

/**
 * Converts from Celsius to Fahrenheit.
 */
function C_to_F(temperature) {
  return Math.round((parseInt(temperature) * (9.0 / 5.0)) + 32);
}

/**
 * Helper function that checks if metric is used in the data for the current
 * city.
 * @param {XMLElement} element forecast_information element from the query.
 * @return {Boolean} True if metric system is used.
 */
function IsMetric(element) {
  var unit = GetData(element, "unit_system");
  if (unit == "SI") {
    return true;
  }
  return false;
}
