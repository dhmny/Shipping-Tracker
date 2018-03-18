var $bg = $('#image_background');
var $text = $('#image_background_text');
var lastValue = null;
function changeImage(value) {
  var choose = 'normal';
  var background = {
    blue: 'http://i.imgur.com/ZE0W7Yx.png',
    normal: 'http://i.imgur.com/uuYygjz.png',
    yellow: 'http://i.imgur.com/RHvDkUE.png'
  }
if (value > 0.8) {
    choose = 'blue';
    $text.text("Normal position.");
    $bg.css('color', 'white');

  } else if (value < -0.8) {
    choose = 'yellow';
    $text.text("Upside-down.");
    $bg.css('color', 'white');
  }
  
  else {
    $text.text("On side or tilted.");

  }
$bg.css('background', 'url(' + background[choose] + ') 0 / contain no-repeat');
}
function getLastValue(variableId, token) {
  var url = 'https://things.ubidots.com/api/v1.6/variables/' + variableId + '/values';
  $.get(url, { token: token, page_size: 1 }, function (res) {
    if (lastValue === null ||res.results[0].value !== lastValue.value) {
      lastValue = res.results[0].value;
      changeImage(lastValue);
    }
  });
}
setInterval(function () {
  getLastValue('5a91e069c03f9733092222cb', 'A1E-QrVm0RfSxsS8QSKFRT6BcdRgWsJaBN');
}, 2000);
