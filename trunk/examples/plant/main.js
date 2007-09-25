plugin.onAddCustomMenuItems = AddCustomMenuItems;

var token = null;
var parts = [];
var grows = 0;

var tints = new Object();
tints['p'] = "#FF67e4";
tints['pu'] = "#a080d0";
tints['r'] = "#ff3737";
tints['y'] = "#ffff11";

/** 
 * Options are as follows:
 * plant: {orchid, cactus, tulip, geranium}
 * color: {p, pu, r, y}
 * pot: {1, 2, 3, 4}
 */

// Set up options and initialize chosen plant
function onOpen() {
  options.putDefaultValue("plant", "orchid");
  options.putDefaultValue("color", "p");
  options.putDefaultValue("pot", "1");

  if (options.getValue("plant") == "orchid") {
    init_orchid();
  } else if (options.getValue("plant") == "cactus") {
    init_cactus();
  } else if (options.getValue("plant") == "geranium") {
    init_geranium();
  } else if (options.getValue("plant") == "tulip") {
    init_tulip();
  }

  token = setInterval(grow, 5000);

  // REMOVE_GADGET_BEGIN
  // if in preview mode don't show the gift card button
  if (options.exists("__isGeneratingGiftPreview"))
    giftCardButton.visible = false;
  // REMOVE_GADGET_END
}

function reset() {
  parts = [];
  grows = 0;
  clearInterval(token);
  plant.removeAllElements();
  _onOpen();
}

function AddCustomMenuItems(menu) {
  menu.AddItem(RESET, 0, reset);
}

// Pick a plant part at random and grow it
function grow() {
  var done = 0;
  var i = 0;
  
  // Stop when the plant is full grown
  if (parts.length == 0) {
    clearInterval(token);
    return;
  }
  
  // Pick a part by random, remove it from list if it's
  // fully grown.
  while (!done && parts.length != 0) {
    i = Math.floor(Math.random() * parts.length);
    if (parts[i].phase < parts[i].phases) {
      done = 1;
      parts[i].grow();
      grows++;
    } else {
      parts[i] = parts[parts.length - 1];
      parts.pop();
    }
  }

  debug.trace("grows: " + grows + " left: " + parts.length);
}

// Set up the orchid
function init_orchid() {
  giftCardButton.x = 93;
  giftCardButton.y = 127;
  pot.x = 50;
  pot.y = 118;
  pot.src = "images\\Pot" + options.getValue("pot") + ".png";
  plant.appendElement("<img src='images\\orchid\\branch.png' x='46' y='10'/>");
  parts.push(new PlantPart('images\\orchid\\leaves1.png', 5, 39, 95));
  
  var x_coord = ["90", "39", "71", "116", "36", "89", "68", "137", "42", "98", "123", "126", "137", "45", "113", "58"];
  var y_coord = ["45", "75", "22", "74", "26", "74", "49", "63", "48", "22", "0", "21", "36", "5", "51", "70"];
  var color = options.getValue("color");
  var flowerPhases = 4;
  var path = "";

  for (var i = 0; i < 16; i++) {
    path = 'images\\orchid\\gray\\Flower' + (i + 1) + '_1.png';
    parts.push(new PlantPart(path, flowerPhases, x_coord[i], y_coord[i],
      tints[color]));
  }
}

// Set up tulip
function init_tulip() {
  pot.x = 47;
  pot.y = 105;
  giftCardButton.x = 100;
  giftCardButton.y = 120;
  pot.src = "images\\Pot" + options.getValue("pot") + ".png";
  
  parts.push(new PlantPart('images\\tulip\\leaves1_1.png', 4, 83, 40));
  plant.appendElement("<img src='images\\tulip\\stems.png' x='65' y='25'/>");
  parts.push(new PlantPart('images\\tulip\\leaves2_1.png', 4, 29, 29));

  var x_coord = ["102", "70", "70", "91", "43", "122"];
  var y_coord = ["12", "17", "6", "0", "6", "3", "29"];
  var color = options.getValue("color");
  var flowerPhases = 5;
  var path = "";

  for (var i = 5; i >= 0; i--) {
    path = 'images\\tulip\\gray\\Flower' + (i + 1) + '_1.png';
    parts.push(new PlantPart(path, flowerPhases, x_coord[i], y_coord[i],
      tints[color]));
  }  
}

// Set up cactus
function init_cactus() {
  pot.x = 45;
  pot.y = 118;
  giftCardButton.x = 87;
  giftCardButton.y = 132;
  pot.src = "images\\Pot" + options.getValue("pot") + ".png";

  plant.appendElement("<img src='images\\cactus\\plant1.png' x='73' y='67'/>");
  plant.appendElement("<img src='images\\cactus\\plant2_5.png' x='46' y='35'/>");
  plant.appendElement("<img src='images\\cactus\\branch.png' x='0' y='0'/>");
  parts.push(new PlantPart('images\\cactus\\plant3_1.png', 4, 67, 0));

  var x_coord = ["48", "79", "87", "101", "118", "22", "39", "49"];
  var y_coord = ["60", "38", "35", "42", "56", "16", "4", "3"];
  var color = options.getValue("color");
  var flowerPhases = 4;
  var path = "";

  for (var i = 0; i < 8; i++) {
    path = 'images\\cactus\\gray\\Flower' + (i + 1) + '_1.png';
    parts.push(new PlantPart(path, flowerPhases, x_coord[i], y_coord[i],
      tints[color]));
  }  
}

// Set up geranium
function init_geranium() {
  pot.x = 53;
  pot.y = 91;
  giftCardButton.x = 96;
  giftCardButton.y = 115;
  pot.src = "images\\Pot" + options.getValue("pot") + ".png";
  parts.push(new PlantPart('images\\geranium\\plant1.png', 7, 0, 26));
  plant.appendElement("<img src='images\\geranium\\branch.png' x='0' y='26'/>");

  x_coord = ["70", "17", "116"];
  y_coord = ["0", "25", "35"];
  var color = options.getValue("color");
  var flowerPhases = 5;
  var path = "";

  for (var i = 0; i < 3; i++) {
    path = 'images\\geranium\\gray\\Flower' + (i + 1) + '_1.png';
    parts.push(new PlantPart(path, flowerPhases, x_coord[i], y_coord[i],
      tints[color]));
  }  
}
   
// Plant part object.  Takes a default image path, number of grow phases, and position.
// Handles plant part actions like image object creation and growing.
function PlantPart(path, p, x, y, tint) {
  var image = plant.appendElement("<img />");
  var oldImage = plant.appendElement("<img opacity='0' />");
  this.phase = 1;
  this.phases = p;
  
  debug.trace("path: " + path + " p: " + p + " x: " + x + " y: " + y);

  image.src = path;
  image.x = x;
  oldImage.x = x;
  image.y = y;
  oldImage.y = y;
  if (tint != null) {
    image.colorMultiply = tint;
    oldImage.colorMultiply = tint;
  }
  
  // Grow a plant part by moving to the next plant phase
  this.grow = function() {
    this.phase++;
    var temp = oldImage;
    var path = image.src.substring(0, image.src.length - 5);
    path += this.phase + ".png";

    oldImage = image;
    image = temp;
    image.src = path;

    beginAnimation(animate, 0, 355, 2500);
  }

  // Animate smoothly to the next image
  function animate() {
    image.opacity = Math.min(event.value, 255);
    oldImage.opacity = Math.min(355 - event.value, 255);
  }

  // REMOVE_GADGET_BEGIN
  // check if preview is being generated and if so show the part in a medium
  // grown state
  if (options.exists("__isGeneratingGiftPreview")) {
    for (var i = 0; i < p / 2; ++i)
      this.grow();
  }
  // REMOVE_GADGET_END  
}
