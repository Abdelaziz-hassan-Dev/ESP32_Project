const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center; }
    h2 { font-size: 2.5rem; }
    p { font-size: 2.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px; }
    
    /* تصميم حالة الحريق */
    .flame-card {
      border: 3px solid #ccc;
      padding: 10px;
      margin: 20px;
      border-radius: 10px;
      background-color: #f9f9f9;
    }
  </style>
</head>
<body>
  <h2>ESP32 Smart Monitor</h2>
  
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temp</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>

  <div class="flame-card">
    <p>
      <i class="fas fa-fire" style="color:#ff4444;"></i>
      <span class="dht-labels">Fire Status:</span>
      <br>
      <span id="flame" style="font-weight:bold;">%FLAME%</span>
    </p>
  </div>

</body>
<script>
// تحديث الحرارة
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 4500 ) ; // كل 5 ثواني

// تحديث الرطوبة
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 4500 ) ;

// تحديث حالة الحريق (أسرع - كل ثانية)
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var status = this.responseText;
      var el = document.getElementById("flame");
      el.innerHTML = status;
      // تغيير اللون حسب الحالة
      if(status.includes("DETECTED")) {
         el.style.color = "red";
         document.body.style.backgroundColor = "#ffcccc"; // وميض للخلفية
      } else {
         el.style.color = "green";
         document.body.style.backgroundColor = "white";
      }
    }
  };
  xhttp.open("GET", "/flame", true);
  xhttp.send();
}, 250 ) ; // كل ثانية
</script>
</html>)rawliteral";