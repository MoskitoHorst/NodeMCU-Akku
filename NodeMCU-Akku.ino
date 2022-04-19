#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <Update.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* host = "Akku";
const char* ssid = "zweiundvierzig";
const char* password = "6199144237657882";
const char* ntpServer = "time.google.com";

const int PIN_LED = 1;
int PIN_MAIN_POWER = 13;
int PIN_VCC   = 36;
int PIN_ADC1  = 39;
int PIN_ADC2  = 34;
int PIN_ADC50 = 35;
int PIN_NT1_POWER = 4;
int PIN_NT1_V220_POWER = 5;
int ACTIVE_LOW_MAIN_POWER = 0;
int ACTIVE_LOW_NT1 = 0;
int ACTIVE_LOW_NT1_V220 = 1;
int ACTIVE_LOW_GTI = 0;
int ACTIVE_LOW_GTI_V220 = 1;
int PIN_GTI_POWER = 27;
int PIN_GTI_POWER_RESISTOR = 14;
int PIN_GTI_V220_POWER = 18;
float vccBatAdjust=0.019368;
int   iAdc1Offset=3000;
int   iAdc2Offset=3023;
int   iAdc50Offset=3000;
float iAdc1Factor=0.008056640625; /*20A, 66mV/A*/
float iAdc2Factor=0.008056640625; /*20A, 66mV/A*/
float iAdc50Factor=0.04028; /*50A, 20mV/A, */
const int samples = 20;
short int adc1Samples[samples];
short int adc2Samples[samples];
short int adc50Samples[samples];
short int vccSamples[samples];

float kWhStoredDC; // kWh, die eingeladen wurden
float kWhRetreavedDC; // kWh, die entnommen wurden
float currentProduction;

WebServer server(80);

void setup(void);

const char weg[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
</body>
<script>function togglePowerSwitch() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("mainPowerSwitch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/mainPowerToggle", true);
  xhttp.send();
};
function toggleGtiSwitch() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("gtiSwitch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/gtiToggle", true);
  xhttp.send();
} ;)rawliteral";

const char indexHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>PV Akku</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Batterie-Spannung</span> 
    <span id="vcc">%VCC%</span>
    Volt
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Uptime</span>
    <span id="uptime"></span>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Time</span>
    <span id="currenttime"></span>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">ADC1</span>
    <span id="adc1">%ADC1%</span>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">ADC2</span>
    <span id="adc2">%ADC2%</span>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">ADC50</span>
    <span id="adc50">%ADC50%</span>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Zähler</span>
    <span id="mains"></span>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Relais</span><br/>
    <span class="dht-labels">Main Power:</span>
    <button id="mainPowerSwitch" onclick="togglePowerSwitch()">---</button>
  </p>
<table border="1">
<tr> <td></td><td>230V</td><td>DC</td></tr>
<tr>
 <td>Inverter</td>
 <td><button id="gtiV220Switch" onclick="toggleGtiV220Switch()">---</button></td>
 <td><button id="gtiSwitch" onclick="toggleGtiSwitch()">---</button></td>
</tr>
<tr>
 <td>Lader</td>
 <td><button id="nt1V220Switch" onclick="toggleNt1V220Switch()">---</button></td>
 <td><button id="nt1Switch" onclick="toggleNt1Switch()">---</button></td>
</tr>
</table>
<br>
<a href="/setup">Setup</a>
</body>
<script>function togglePowerSwitch() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("mainPowerSwitch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/mainPowerToggle", true);
  xhttp.send();
};
function toggleGtiSwitch() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("gtiSwitch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/gtiToggle", true);
  xhttp.send();
} ;
function toggleGtiV220Switch() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("gtiV220Switch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/gtiV220Toggle", true);
  xhttp.send();
} ;
function toggleNt1Switch() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("nt1Switch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/nt1Toggle", true);
  xhttp.send();
} ;
function toggleNt1V220Switch() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("nt1V220Switch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/nt1V220Toggle", true);
  xhttp.send();
} ;


setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("vcc").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/vcc", true);
  xhttp.send();
}, 500 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("uptime").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/values?name=uptime", true);
  xhttp.send();
}, 530 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("currenttime").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/currenttime", true);
  xhttp.send();
}, 535 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("adc1").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/ADC1", true);
  xhttp.send();
}, 560 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("adc2").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/ADC2", true);
  xhttp.send();
}, 590 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("mains").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/mains", true);
  xhttp.send();
}, 591 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("adc50").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/ADC50", true);
  xhttp.send();
}, 620 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("mainPowerSwitch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/mainPowerSwitch", true);
  xhttp.send();
}, 630 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("gtiSwitch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/gtiSwitchState", true);
  xhttp.send();
}, 640 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("gtiV220Switch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/gtiV220SwitchState", true);
  xhttp.send();
}, 650 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("nt1Switch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/nt1SwitchState", true);
  xhttp.send();
}, 660 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("nt1V220Switch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/nt1V220SwitchState", true);
  xhttp.send();
}, 670 ) ;
</script>
</html>)rawliteral";

// --------------------------------------------------------------------------------------------------------

const char setupHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body onload="loaded();">
  <h2>PV Akku Setup</h2>
  <p>
    <span class="dht-labels">Batterie-Spannung</span> 
    <span id="vcc"></span>
    Volt
  </p>
  <h3 id="inputCalChannel"></h3>
  <h1 id="inputCalVolts"></h1>
  <p>
    <span>calibration factor: </span>
    <input id="inputCalCal" type="number" onchange="inputCalCal(this)" min="5" max="300" step="0.01" width="100px"></p>
  <p>
    <button class="actionCancel" onclick="calVTexit(this)">cancel</button>
    <button class="actionSave" onclick="calVTsave(this)">save</button>
  </p>
  <br>
  Using a voltmeter to display the battery voltage, adjust the calibration factor until the displayed voltage 
  reasonably matches the voltmeter reading. Click save to update the channel with the new calibration factor.
</body>
<script>

function loaded(){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("inputCalCal").value = this.responseText;
    }
  };
  xhttp.open("GET", "/values?name=vccAdj", true);
  xhttp.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("vcc").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/vcc", true);
  xhttp.send();
}, 500 ) ;

</script>
</html>)rawliteral";

// --------------------------------------------------------------------------------------------------------


int i=0;

void handleRoot() {
  char txt[300];
  int vcci;
  server.send(200, "text/html", indexHtml);
}

void handleSetup() {
  char txt[300];
  int vcci;
  server.send(200, "text/html", setupHtml);
}

void handleValues() {
  String pname;
  char txt[40];
  pname=server.arg("name");
  if(!pname){
    server.send(400, "text/plain", "Parameter Name fehlt.");
  }
  else{
    if(!strcasecmp(pname.c_str(),"idx")){
      sprintf(txt,"%d",i);
    }
    else if(!strcasecmp(pname.c_str(),"Uptime")){
      int t, s, m, h, d;
      t= millis()/1000;
      s=t%60;
      t=t/60;
      m=t%60;
      t=t/60;
      h=t%24;
      t=t/24;
      d=t;
      sprintf(txt,"%d Tage %d:%02d:%02d",d,h,m,s);
    }
    else if(!strcasecmp(pname.c_str(),"vccAdj")){
      sprintf(txt,"%f",1/vccBatAdjust);
    }
    else{
      sprintf(txt,"ungueltiger name %s",pname);
    }
    server.send(200, "text/plain", txt);
  }
}

void handleIdx() {
  char txt[20];
  sprintf(txt,"%d",i);
  server.send(200, "text/plain", txt);
}

void handleUptime() {
  char txt[50];
  int t, s, m, h, d;
  t= millis()/1000;
  s=t%60;
  t=t/60;
  m=t%60;
  t=t/60;
  h=t%24;
  t=t/24;
  d=t;
  sprintf(txt,"%d Tage %d:%02d:%02d",d,h,m,s);
  server.send(200, "text/plain", txt);
}

void handleCurrenttime() {
  char buffer[50];
  struct tm time;
  int t, s, m, h, d;
  buffer[0]=0;
  if(getLocalTime(&time)){
    strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", &time);
  }
  server.send(200, "text/plain", buffer);
}

void handleVcc() {
  char txt[20];
  int vcci, i;
  // vcci=analogRead(PIN_VCC);
  for(vcci=0,i=0;i<samples;i++) vcci += vccSamples[i];
  vcci= vcci / samples;
  sprintf(txt,"%.2f",vcci*vccBatAdjust);
  server.send(200, "text/plain", txt);
}

void handleADC1() {
  char txt[20];
  int adc, i;
  // adc=analogRead(PIN_ADC1);
  for(adc=0,i=0;i<samples;i++) adc += adc1Samples[i];
  adc= adc / samples;
  sprintf(txt,"%.3f",(adc-iAdc1Offset)*iAdc1Factor);
  server.send(200, "text/plain", txt);
}

void handleADC2() {
  char txt[20];
  int adc, i;
  // adc=analogRead(PIN_ADC2);
  for(adc=0,i=0;i<samples;i++) adc += adc2Samples[i];
  adc= adc / samples;
  sprintf(txt,"%.2f",(adc-iAdc2Offset)*iAdc2Factor);
  server.send(200, "text/plain", txt);
}

void handleADC50() {
  char txt[20];
  int adc, i;
  // adc=analogRead(PIN_ADC50);
  for(adc=0,i=0;i<samples;i++) adc += adc50Samples[i];
  adc= adc / samples;
  sprintf(txt,"%.2f",(adc-iAdc50Offset)*iAdc50Factor);
  server.send(200, "text/plain", txt);
}

void handleMains() {
  char txt[20];
  sprintf(txt,"%.0f",currentProduction);
  server.send(200, "text/plain", txt);
}

void handlePowerSwitchToggle() {
  char txt[20];
  int status = digitalRead(PIN_MAIN_POWER);
  digitalWrite(PIN_MAIN_POWER, (status)?0:1);
  sprintf(txt,(status)?"Off":"On");
  server.send(200, "text/plain", txt);
}

void handlePowerOn() {
  char txt[20];
  digitalWrite(PIN_MAIN_POWER, 1);
  sprintf(txt,"ok\n");
  server.send(200, "text/plain", txt);
}

void handlePowerOff() {
  char txt[20];
  digitalWrite(PIN_MAIN_POWER, 0);
  sprintf(txt,"ok\n");
  server.send(200, "text/plain", txt);
}

void handleNt1On() {
  char txt[20];
  digitalWrite(PIN_NT1_POWER, ACTIVE_LOW_NT1 ^ 1);
  sprintf(txt,"ok\n");
  server.send(200, "text/plain", txt);
}

void handleNt1Off() {
  char txt[20];
  digitalWrite(PIN_NT1_POWER, ACTIVE_LOW_NT1 ^ 0);
  sprintf(txt,"ok\n");
  server.send(200, "text/plain", txt);
}

void handleNt1SwitchToggle() {
  char txt[20];
  int status = digitalRead(PIN_NT1_POWER);
  if(ACTIVE_LOW_NT1 ^ status){
    digitalWrite(PIN_NT1_POWER, ACTIVE_LOW_NT1 ^ 0);
  }
  else{
    digitalWrite(PIN_NT1_POWER, ACTIVE_LOW_NT1 ^ 1);
  }
  sprintf(txt,"%s\n",(ACTIVE_LOW_NT1 ^ status)?"Off":"On");
  server.send(200, "text/plain", txt);
}

void handleNt1V220SwitchToggle() {
  char txt[20];
  int status = digitalRead(PIN_NT1_V220_POWER);
  if(ACTIVE_LOW_NT1_V220 ^ status){
    digitalWrite(PIN_NT1_V220_POWER, ACTIVE_LOW_NT1_V220 ^ 0);
  }
  else{
    digitalWrite(PIN_NT1_V220_POWER, ACTIVE_LOW_NT1_V220 ^ 1);
  }
  sprintf(txt,"%s\n",(ACTIVE_LOW_NT1_V220 ^ status)?"Off":"On");
  server.send(200, "text/plain", txt);
}

void handleGtiOn() {
  char txt[20];
  digitalWrite(PIN_GTI_POWER_RESISTOR, ACTIVE_LOW_GTI ^ 1);
  delay(1000);
  digitalWrite(PIN_GTI_POWER, ACTIVE_LOW_GTI ^ 1);
  sprintf(txt,"ok\n");
  server.send(200, "text/plain", txt);
}

void handleGtiOff() {
  char txt[20];
  digitalWrite(PIN_GTI_POWER, ACTIVE_LOW_GTI ^ 0);
  digitalWrite(PIN_GTI_POWER_RESISTOR, ACTIVE_LOW_GTI ^ 0);
  sprintf(txt,"ok\n");
  server.send(200, "text/plain", txt);
}

void handleGtiSwitchToggle() {
  char txt[20];
  int status = digitalRead(PIN_GTI_POWER);
  if(ACTIVE_LOW_GTI ^ status){
    digitalWrite(PIN_GTI_POWER, ACTIVE_LOW_GTI ^ 0);
    digitalWrite(PIN_GTI_POWER_RESISTOR, ACTIVE_LOW_GTI ^ 0);
  }
  else{
    digitalWrite(PIN_GTI_POWER_RESISTOR, ACTIVE_LOW_GTI ^ 1);
    delay(1000);
    digitalWrite(PIN_GTI_POWER, ACTIVE_LOW_GTI ^ 1);
  }
  sprintf(txt,"%s\n",(ACTIVE_LOW_GTI ^ status)?"Off":"On");
  server.send(200, "text/plain", txt);
}

void handleGtiV220SwitchToggle() {
  char txt[20];
  int status = digitalRead(PIN_GTI_V220_POWER);
  if(ACTIVE_LOW_GTI_V220 ^ status){
    digitalWrite(PIN_GTI_V220_POWER, ACTIVE_LOW_GTI_V220 ^ 0);
  }
  else{
    digitalWrite(PIN_GTI_V220_POWER, ACTIVE_LOW_GTI_V220 ^ 1);
  }
  sprintf(txt,"%s\n",(ACTIVE_LOW_GTI_V220 ^ status)?"Off":"On");
  server.send(200, "text/plain", txt);
}

void handleMainPowerSwitchStatus() {
  char txt[20];
  int status = digitalRead(PIN_MAIN_POWER);
  sprintf(txt,"%s\n",(status)?"On":"Off");
  server.send(200, "text/plain", txt);
}

void handleGtiSwitchStatus() {
  char txt[20];
  int status = digitalRead(PIN_GTI_POWER);
  sprintf(txt,"%s\n",(ACTIVE_LOW_GTI ^ status)?"On":"Off");
  server.send(200, "text/plain", txt);
}

void handleGtiV220SwitchStatus() {
  char txt[20];
  int status = digitalRead(PIN_GTI_V220_POWER);
  sprintf(txt,"%s\n",(ACTIVE_LOW_GTI_V220 ^ status)?"On":"Off");
  server.send(200, "text/plain", txt);
}

void handleNt1SwitchStatus() {
  char txt[20];
  int status = digitalRead(PIN_NT1_POWER);
  sprintf(txt,"%s\n",(ACTIVE_LOW_NT1 ^ status)?"On":"Off");
  server.send(200, "text/plain", txt);
}

void handleNt1V220SwitchStatus() {
  char txt[20];
  int status = digitalRead(PIN_NT1_V220_POWER);
  sprintf(txt,"%s\n",(ACTIVE_LOW_NT1_V220 ^ status)?"On":"Off");
  server.send(200, "text/plain", txt);
}


String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "--"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, 0);
  pinMode(PIN_MAIN_POWER, OUTPUT);
  digitalWrite(PIN_MAIN_POWER, 0);
  pinMode(PIN_GTI_POWER, OUTPUT);
  digitalWrite(PIN_GTI_POWER, ACTIVE_LOW_GTI ^ 0);
  pinMode(PIN_GTI_POWER_RESISTOR, OUTPUT);
  digitalWrite(PIN_GTI_POWER_RESISTOR, ACTIVE_LOW_GTI ^ 0);
  pinMode(PIN_GTI_V220_POWER, OUTPUT);
  digitalWrite(PIN_GTI_V220_POWER, ACTIVE_LOW_GTI_V220 ^ 0);
  pinMode(PIN_NT1_POWER, OUTPUT);
  digitalWrite(PIN_NT1_POWER, ACTIVE_LOW_NT1 ^ 0);
  pinMode(PIN_NT1_V220_POWER, OUTPUT);
  digitalWrite(PIN_NT1_V220_POWER, ACTIVE_LOW_NT1_V220 ^ 0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    while (1) {
      delay(1000);
    }
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(host);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      ;
    });

  ArduinoOTA.begin();

  server.on("/", handleRoot);
  server.on("/setup", handleSetup);
  server.on("/values", handleValues);
  server.on("/idx", handleIdx);
  server.on("/uptime", handleUptime);
  server.on("/currenttime", handleCurrenttime);
  server.on("/vcc", handleVcc);
  server.on("/ADC1", handleADC1);
  server.on("/ADC2", handleADC2);
  server.on("/ADC50", handleADC50);
  server.on("/mains", handleMains);
  server.on("/mainPowerSwitch", handleMainPowerSwitchStatus);
  server.on("/powerOn", handlePowerOn);
  server.on("/powerOff", handlePowerOff);
  server.on("/mainPowerToggle", handlePowerSwitchToggle);
  server.on("/gtiOn", handleGtiOn);
  server.on("/gtiOff", handleGtiOff);
  server.on("/gtiToggle", handleGtiSwitchToggle);
  server.on("/gtiV220Toggle", handleGtiV220SwitchToggle);
  server.on("/gtiSwitchState", handleGtiSwitchStatus);
  server.on("/gtiV220SwitchState", handleGtiV220SwitchStatus);
  server.on("/nt1On", handleNt1On);
  server.on("/nt1Off", handleNt1Off);
  server.on("/nt1Toggle", handleNt1SwitchToggle);
  server.on("/nt1V220Toggle", handleNt1V220SwitchToggle);
  server.on("/nt1SwitchState", handleNt1SwitchStatus);
  server.on("/nt1V220SwitchState", handleNt1V220SwitchStatus);
  // server.on("/index.html", handleRoot);
  server.begin();

  configTime(0, 3600, ntpServer);
  // MDNS.addService("http", "tcp", 80);
}

int lastTime;

void loop() {
  int offs, newTime, deltaTime;
  ArduinoOTA.handle();
  server.handleClient();
  delay(2);
  newTime=millis();
  if((newTime-lastTime<100) && (newTime>lastTime)){
    deltaTime=newTime-lastTime;
  }
  lastTime=newTime;
  if(!(i++ & 511)){
    String power;
    digitalWrite(PIN_LED, (i&1024)?HIGH:LOW);
    power = httpGETRequest("http://iotawatt.fritz.box/query?select=[Leistung]&begin=s-10s&end=s&group=all&format=csv");
    currentProduction = power.toFloat();
  }
  offs = i % samples;
  adc1Samples[offs] = analogRead(PIN_ADC1);
  adc2Samples[offs] = analogRead(PIN_ADC2);
  adc50Samples[offs] = analogRead(PIN_ADC50);
  vccSamples[offs] = analogRead(PIN_VCC);
}
