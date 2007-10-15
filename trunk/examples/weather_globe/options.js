// Special vars
var kUnitedStates = "US";
var kSelectedcity = "Selected city: ";

// String labels
var kCountryDropBox = "coutry dropbox";
var kCityDropBox = "city dropbox";
var kCityEditBox = "city editbox";
var kCityButton = "city button";
var kChosenCityLabel = "chosen city label";
var kFarenheitButton = "farenheit";
var kCelsiusButton = "celsius";

var kLocationSetting = "location setting";
var kCityName = "city name";
var kLatLong = "latitude longitude";
var kZipcode = "zipcode";
var kDefaultLatLong = BuildLatLong(0, 0);
var kDefaultZip = 0;
var kUseMetric = "use metric";

// Options fetch state
var OptionsFetchType = { Empty: 0,
                         FetchCountry: 1, CountryListReceived: 2,
                         FetchCities : 3, CityListReceived: 4,
                         VerifyCity: 5 };

var optionsFetchState = OptionsFetchType.Empty;
var isoCodes = null;
var countryNames = null;
var latLongList = null;

var cityRequest = false;

// Persistent settings
options.putDefaultValue(kLocationSetting, kZipcode); // { kZipcode || kLatLong || kCityName }
options.putDefaultValue(kCityName, strings.DEFAULT_CITY);
options.putDefaultValue(kLatLong, kDefaultLatLong);
options.putDefaultValue(kZipcode, 79042); // Happy, TX
options.putDefaultValue(kUseMetric, strings.DEFAULT_USE_METRIC);
options.putDefaultValue("TrayOpen", false);
options.putDefaultValue("language", strings.DEFAULT_LANGUAGE);
options.putDefaultValue("Country", kUnitedStates); // ISO code

//
// OPTIONS FUNCTIONS
//
plugin.onShowOptionsDlg = ShowOptions;

// Lay out options components, start populating country list
function ShowOptions(window) {

  // Instructions
  window.AddControl(gddWndCtrlClassLabel, 0, "", strings.OPTIONS_INSTRUCTIONS,
                    20, 15, 370, 90);

  // Country drop list
  window.AddControl(gddWndCtrlClassLabel, 0, "", strings.OPTIONS_COUNTRY,
                    20, 109, 120, 20);
  var countryDropList = window.AddControl(gddWndCtrlClassList, gddWndCtrlTypeListDrop,
                                          kCountryDropBox, new Array(), 150, 105, 220, 360);
  countryDropList.onChanged = OnClickCountryList;

  // City, State edit box
  window.AddControl(gddWndCtrlClassLabel, 0, "", strings.OPTIONS_CITY_STATE,
                    20, 145, 120, 30);
  var cityEditBox = window.AddControl(gddWndCtrlClassEdit, 0, kCityEditBox,
                                      "", 150, 140, 220, 23);

  // Fill the edit box with the current city if possible
  if (options.getValue("Country") == kUnitedStates)
    cityEditBox.value = options.getValue(kCityName);

  // City drop list
  window.AddControl(gddWndCtrlClassLabel, 0, "", strings.OPTIONS_CITY,
                    20, 182, 120, 20);
  var citiesDropList = window.AddControl(gddWndCtrlClassList, gddWndCtrlTypeListDrop, kCityDropBox,
                                         new Array(), 150, 180, 220, 360);

  // Preferentially enable the city edit box. It'll get updated once we receive
  // the country list.
  citiesDropList.enabled = true;
  cityEditBox.enabled = false;

  // Temperature unit buttons
  var farenheitButton = window.AddControl(gddWndCtrlClassButton,
      gddWndCtrlTypeButtonCheck, kFarenheitButton, strings.OPTIONS_FAHRENHEIT, 50, 230, 100, 20);
  var celsiusButton = window.AddControl(gddWndCtrlClassButton,
      gddWndCtrlTypeButtonCheck, kCelsiusButton, strings.OPTIONS_CELSIUS, 160, 230, 100, 20);
  farenheitButton.onClicked = OnMetricChoiceChanged;
  celsiusButton.onClicked = OnMetricChoiceChanged;

  if (options.getValue(kUseMetric)) {
    celsiusButton.value = true;
  } else {
    farenheitButton.value = true;
  }

  GetCountryList(window, countryDropList);

  window.onClose = OnOptionsClose;

  function OnOptionsClose(window, code) {
    if (code == gddIdOK) {
      // Update options, verify input and update the pane
      var countryName = window.GetControl(kCountryDropBox).value;
      if (!countryName)
        return;  // Country list wasn't fetched
      options.putValue("Country", FindISOCode(countryName));

      if (cityEditBox.enabled) {
        VerifyCityAndInit(window);
      } else {
        var cityDropBox = window.GetControl(kCityDropBox);
        ChooseCityFromList(window, cityDropBox.value);
        Weather_Init();
      }
    }
  }

  // Toggle between celsius and farenheit
  function OnMetricChoiceChanged(window, control) {
    if (control.id == kFarenheitButton) {
      if (control.value) {
        ChooseFarenheit();
      } else {
        ChooseCelsius();
      }
    }
    if (control.id == kCelsiusButton) {
      if (control.value) {
        ChooseCelsius();
      } else {
        ChooseFarenheit();
      }
    }
  }

  // Toggle the checkboxes
  function ChooseFarenheit() {
    options.putValue(kUseMetric, false);
    farenheitButton.value = true;
    celsiusButton.value = false;
  }

  function ChooseCelsius() {
    options.putValue(kUseMetric, true);
    farenheitButton.value = false;
    celsiusButton.value = true;
  }

} // End ShowOptions


//
// REQUEST CALLBACKS
//

function GetCountryList(window, dropList) {
  optionsFetchState = OptionsFetchType.FetchCountry;

  var url = kGoogleURI + kCountriesUrl + options.getValue("language");
  var countryRequest = new XMLHttpRequest();
  countryRequest.onreadystatechange = OnGetCountryList;
  countryRequest.open("GET", url, true);
  countryRequest.send();

  function OnGetCountryList() {
    if (countryRequest.readyState == 4 && countryRequest.status == 200) {
      var doc = new DOMDocument();
      doc.loadXML(countryRequest.responseText);

      try {
        var countryTags = doc.getElementsByTagName("countries")[0].getElementsByTagName("country");
        countryNames = new Array();
        isoCodes = new Array();

        for (var i = 0; i < countryTags.length; ++i) {
          var name = GetData(countryTags[i], "name");
          var iso_code = GetData(countryTags[i], "iso_code");
          countryNames[i] = name;
          isoCodes[i] = { name: name, iso: iso_code };
        }
        countryNames.sort();
        dropList.text = countryNames;
        optionsFetchState = OptionsFetchType.CountryListReceived;
        var prevCountry = FindCountryName(options.getValue("Country"));
        if (prevCountry) {
          dropList.value = prevCountry;
        }
        GetCityList(window, dropList.value);

      } catch (e) {
        debug.error("Error loading country list: " + e.message);
      }
    }
  }
}

// Fetch the list of cities for the selected country
function OnClickCountryList(window, control) {
  var cityDropBox = window.GetControl(kCityDropBox);
  cityDropBox.text = new Array();
  GetCityList(window, control.value);
}

function GetCityList(window, country) {
  if (optionsFetchState >= OptionsFetchType.CountryListReceived) {
    var iso = FindISOCode(country);

    // If US, ask for city, state
    if (iso == kUnitedStates) {
      var cityEditBox = window.GetControl(kCityEditBox);
      cityEditBox.enabled = true;
      var cityDropBox = window.GetControl(kCityDropBox);
      cityDropBox.enabled = false;
    } else {
      var cityDropBox = window.GetControl(kCityDropBox);
      cityDropBox.enabled = true;
      var cityEditBox = window.GetControl(kCityEditBox);
      cityEditBox.enabled = false;

      if (iso) {
        var url = kGoogleURI + kCitiesUrl + options.getValue("language") + 
          kCountryParam + iso;
        if (cityRequest) {
          cityRequest.abort();
        } else {
          cityRequest = new XMLHttpRequest();
        }
        cityRequest.onreadystatechange = OnGetCityList;
        cityRequest.open("GET", url, true);
        cityRequest.send();
      }
    }

    function OnGetCityList() {
      if (cityRequest.readyState == 4 && cityRequest.status == 200) {
        var doc = new DOMDocument();
        doc.loadXML(cityRequest.responseText);
        try {
          var cityTags = doc.getElementsByTagName("cities")[0].getElementsByTagName("city");
          var cityNames = new Array();
          latLongList = new Array();
          for (var j = 0; j < cityTags.length; ++j) {
            var cityName = GetData(cityTags[j], "name");
            cityNames[j] = cityName;
            var lat      = GetData(cityTags[j], "latitude_e6");
            var longitude     = GetData(cityTags[j], "longitude_e6");
            latLongList[j] = { name: cityName, lat: lat, longitude: longitude };
          }
          cityNames.sort();
          cityDropBox.text = cityNames;
          optionsFetchState = OptionsFetchType.CityListReceived;

          // Show the previously selected city in the droplist if possible
          var countryDropBox = window.GetControl(kCountryDropBox);
          if (FindISOCode(countryDropBox.value) == options.getValue("Country")) {
            var prevCity = options.getValue(kCityName);
            if (cityNames.exists(prevCity)) {
              cityDropBox.value = prevCity;
            }
          }
        } catch (e) {
          debug.error("Error loading city list: " + e.message);
        }
      }
    }
  }
} // End OnClickCountryList


// Choose a city
function ChooseCityFromList(window, city) {
  var latlong = FindLatLong(city);
  if (latlong) {
    options.putValue(kLatLong, BuildLatLong(latlong.lat, latlong.longitude));
    options.putValue(kZipcode, kDefaultZip);
    options.putValue(kLocationSetting, kLatLong);
    options.putValue(kCityName, city);
  }
}

// Verify the city, state combo is valid. Reinit the weather if valid.
function VerifyCityAndInit(window) {
  optionsFetchState = OptionsFetchType.VerifyCity;

  var editBox = window.GetControl(kCityEditBox);
  var url = kGoogleURI + kWeatherUrl + options.getValue("language") + 
    kWeatherQueryParam + editBox.text;

  var verifyRequest = new XMLHttpRequest();
  verifyRequest.onreadystatechange = OnVerifyCityReceived;
  debug.trace("sending query to "+url);
  verifyRequest.open("GET", url, true);
  verifyRequest.send();

  function OnVerifyCityReceived() {
    if (verifyRequest.readyState == 4 && verifyRequest.status == 200) {
      var doc = new DOMDocument();
      doc.loadXML(verifyRequest.responseText);
      try {
        var weatherTags = doc.getElementsByTagName("weather");
        var problem = weatherTags[0].getElementsByTagName("problem_cause");
        if (problem && problem.length == 1) {
          alert(strings.INVALID_OPTION_MSG);
        } else {
          var forecast_info = weatherTags[0].getElementsByTagName("forecast_information")[0];
          options.putValue(kCityName, GetData(forecast_info, "city"));
          options.putValue(kZipcode, GetData(forecast_info, "postal_code"));
          if (options.getValue(kZipcode) == kDefaultZip) {
            options.putValue(kLocationSetting, kCityName);
            debug.trace("no zipcode, using city name: "+options.getValue(kCityName));
          } else {
            options.putValue(kLocationSetting, kZipcode);
          }
          options.putValue(kLatLong, kDefaultLatLong);

          // City looks ok -- lets init the weather panel
          Weather_Init();
        }

      } catch (e) {
        debug.error("Error verifying city: " + e.message);
        alert(strings.INVALID_OPTION_MSG);
      }
    }
  }
}

//
// HELPER FUNCTIONS
//

// Array.exists
Array.prototype.exists = function (match) {
  for (var i = 0; i < this.length; i++) {
    if (this[i] == match) {
     return true;
    }
  }
  return false;
}

// Find the iso code for country from the iso array
function FindISOCode(country) {
  if (isoCodes == null || isoCodes.length == 0) {
    debug.error("CountryName list is empty!");
    return false;
  }

  for (var i = 0; i < isoCodes.length; ++i) {
    if (isoCodes[i].name == country) {
      return isoCodes[i].iso;
    }
  }

  return false;
}

// Find the country name from the iso code
function FindCountryName(iso_code) {
  if (isoCodes.length == 0) {
    debug.error("CountryName list is empty!");
    return false;
  }

  for (var i = 0; i < isoCodes.length; ++i) {
    if (isoCodes[i].iso == iso_code) {
      return isoCodes[i].name;
    }
  }

  return false;
}

// Find the lat and long for city from the latlong array
function FindLatLong(city) {
  if (!latLongList) {
    debug.error("Lat Long list is empty");
    return false;
  }

  for (var i = 0; i < latLongList.length; ++i) {
    if (latLongList[i].name == city) {
      return latLongList[i];
    }
  }

  debug.error("Couldn't find the city name");
  return false;
}

// convert lat long pair to a string
function BuildLatLong(lat, longitude){
  return lat + kIsoLatLongSeparator + longitude;
}
