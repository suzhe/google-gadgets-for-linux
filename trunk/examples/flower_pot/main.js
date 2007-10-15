var kMaxFlowerState = 5;
var kMaxLeaveState  = 4;
var kNumTries       = 10;
var kBonusPets      = 100;
var kAbandonTries   = 9600; // Shrink once a day

var mouseOverCount  = 0;
var kShakeLimit     = 1;
var kMaxRainOpacity = 200;

var kFadeOutLength  = 1500;
var kFadeInLength   = 1500;
var kFadeOutOpacity = 0;
var kFadeInOpacity  = 0;
var kMaxOpacity     = 255;

var animationTimer = 0;
var randomNumbers   = null;

var random_i        = 0;

var kGrowTime = 9000; // 9 seconds

var names  = ["leaves1", "leaves2",
              "Flower1", "Flower2", "Flower3",
              "Flower4", "Flower5", "Flower6"];

var imgs   = ["leaves1", "leaves2",
              "Flower1", "Flower2", "Flower3",
              "Flower3", "Flower4", "Flower5"];

//
// Initialize options
//
var kPetCount = "pet count";
var kAbandonCount = "abandon";
for (var i = 0; i < names.length; ++i) {
  options.putDefaultValue(names[i], "1");
}
options.putDefaultValue(kPetCount, "0");
options.putDefaultValue(kAbandonCount, "0");

//
// Initialize menu
//
/*
pluginHelper.onAddCustomMenuItems = function(menu) {
  menu.AddItem(MENU_RESET, 0, OnReset);
}
*/

//
// OnReset
//
function OnReset(text) {
  options.removeAll();
  view_init();
}

//
// view_init
//
function view_init() {
  for (var i = 0; i < names.length; ++i) {
    view.children.item(names[i]).src = GetImgName(i);
  }
}

//
// view_onopen
//
function view_onopen() {
  view_init();
  setInterval(growNext, kGrowTime);
}

//
// flowers_onmouseover
//
function flowers_onmouseover() {
  var pet_count = GetPetCount() + 1;
  SetPetCount(pet_count);
  SetAbandonCount(0);

  if (pet_count > kBonusPets) {
    growNext();
  }

  // rain settings
  if (mouseOverCount >= kShakeLimit) {
    return;
  }

  mouseOverCount++;
  resetAnimation();
  setTimeout(startFadeOut, 3550);
  if (mouseOverCount == 1) {
    beginAnimation(FadeRain, 0, kMaxRainOpacity, 500);
  }

  animationTimer = beginAnimation("_animate(animated)", 0, 500, 4000);
}

function growNext() {
  var growth;
  var pet_count = GetPetCount();
  var abandon_count = GetAbandonCount();
  if (pet_count > 0) {
    growth = 1;
  } else if (abandon_count > kAbandonTries) {
    growth = -1;
    SetAbandonCount(0);
  } else {
    SetAbandonCount(abandon_count + 1);
    return;
  }

  SetPetCount(0);

  var index, state;
  var i;
  for (i = 0; i < kNumTries; ++i) {
    index = GetNextRandom();
    state = GetState(index);
    if (growth > 0) {
      if ((index < 2 && state < kMaxLeaveState) ||
          (index >= 2 && state < kMaxFlowerState)) {
        break;
      }
    } else { // shrinking
      if (state > 1) {
        break;
      }
    }
  }

  if (i == kNumTries) {
    return;
  }

  state += growth;
  SetState(index, state);

  view.insertElement(GetNextElementXML(index), view.children.item(names[index]));

  beginAnimation("_FadePlant(\""+names[index]+"\")",
                 kMaxOpacity, kFadeOutOpacity, kFadeOutLength);
  beginAnimation("_FadePlant(\""+GetTempElemName(index)+"\")",
                 kFadeInOpacity, kMaxOpacity, kFadeInLength);

  setTimeout("_RemoveNextElementXML("+index+")", kFadeInLength + 500);
}

//
// ANIMATE RAIN
//
function resetAnimation() {
  clearInterval(animationTimer);
  animated.opacity = 175;
}

function _animate(element) {
  for (var i = 0; i < element.children.count; i++) {
    var drop = element.children.item(i);
    drop.y = Math.abs((drop.y + 1) % (element.height - 11));

    var delta = GetRandomOpacityDelta();
    var opacity = drop.opacity + delta;
    if (opacity < 255 && opacity > kMaxRainOpacity - 25) {
      drop.opacity = opacity;
    }
  }
  /*
  var e = new Enumerator(element.children);
  for (; !e.atEnd(); e.moveNext()) {
    e.item().y = Math.abs((e.item().y + 1) % (element.height - 11));

    var delta = GetRandomOpacityDelta();
    var opacity = e.item().opacity + delta;
    if (opacity < 255 && opacity > kMaxRainOpacity - 25) {
      e.item().opacity = opacity;
    }
  }
  */
}

function _FadePlant(name) {
  view.children.item(name).opacity = event.value;
}

// Gracefully end the rain
function startFadeOut() {
  if (--mouseOverCount == 0) {
    beginAnimation(FadeRain, kMaxRainOpacity, 0, 500);
  }
}

function FadeRain() {
  animated.opacity = event.value;
}


function GetRandomOpacityDelta() {
  // Only generate random numbers once
  randomNumbers = [];
  if (randomNumbers == null) {
    for (var i = 0; i < 25; ++i) {
      var rand = Math.random();
      randomNumbers[i] = Math.round((rand * 10.0) / 2.0);
    }
  }

  random_i = (random_i + 1) % 25;
  return randomNumbers[random_i];
}


//
// Util functions
//

function GetNextRandom() {
  var max = names.length - 1;
  return Math.round((Math.random() * (max)) / 1.0);
}

function GetNextElementXML(index) {
  var element = view.children.item(names[index]);
  var ret = "<img name=\"" + GetTempElemName(index) + "\"";
  ret += " src=\"" + GetImgName(index) + "\"";
  ret += " x=\"" + element.x + "\" y=\"" + element.y + "\"";
  ret += " opacity=\"" + kFadeInOpacity + "\"";
  ret += " />";
  return ret;
}

function _RemoveNextElementXML(index) {
  view.children.item(names[index]).src = GetImgName(index);
  view.children.item(names[index]).opacity = kMaxOpacity;
  view.removeElement(view.children.item(GetTempElemName(index)));
}


function GetImgName(index) {
  return imgs[index] + "_" + GetState(index) + ".png";
}

function GetTempElemName(index) {
  return names[index] + "_";
}

function GetState(index) {
  return Number(options.getValue(names[index]));
}

function SetState(index, state) {
  options.putValue(names[index], String(state));
}

function GetPetCount() {
  return Number(options.getValue(kPetCount));
}

function SetPetCount(count) {
  options.putValue(kPetCount, String(count));
}

function GetAbandonCount() {
  return Number(options.getValue(kAbandonCount));
}

function SetAbandonCount(count) {
  options.putValue(kAbandonCount, String(count));
}
