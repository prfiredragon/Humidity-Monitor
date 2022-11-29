const char HTML_MAIN[] PROGMEM = R"rawliteral(
<figure class="text-center">
    <h2 id="devicename"></h2>
</figure>
<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Humididy Percentage:</div>
    </div>
    <div class="col">
        <div class="bg-light"><span id="packV"></span></div>
    </div>
</div>
<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Humidity Value: </div>
    </div>
    <div class="col">
        <div class="bg-light"><span id="packRes"></span></div>
    </div>
</div>



<div class="d-grid gap-2">
    <a class="btn btn-primary btn-block" href="/settings" role="button">Settings</a>
</div>

<script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;
    window.addEventListener('load', onLoad);
    function initWebSocket() {
        console.log('Trying to open a WebSocket connection...');
        websocket = new WebSocket(gateway);
        websocket.onopen = onOpen;
        websocket.onclose = onClose;
        websocket.onmessage = onMessage;
    }
    function onOpen(event) {
        console.log('Connection opened');
        //websocket.send('dataRequired');
    }
    function onClose(event) {
        console.log('Connection closed');
        setTimeout(initWebSocket, 2000);
    }
    function onMessage(event) {
        var data = JSON.parse(event.data);
        document.getElementById("devicename").innerHTML = 'Device: ' + data.Pack.Device_Name;
        document.getElementById("packV").innerHTML = data.Pack.percentage == null ? 'No connection or Sleeping' : data.Pack.percentage + ' \u2030 ';
        document.getElementById("packRes").innerHTML = data.Pack.Value == null ? '&#8205' : data.Pack.Value + ' ';
    }

    function onLoad(event) {
        initWebSocket();
        initButton();
    }

    function initButton() {
        document.getElementById('chargeFetState').addEventListener('click', ChargeFetSwitch);
        document.getElementById('disChargeFetState').addEventListener('click', DischargeFetSwitch);
    }

    function ChargeFetSwitch() {
        let switchVal;
        if (document.getElementById('chargeFetState').checked) { switchVal = 'chargeFetSwitch_on' }
        else { switchVal = 'chargeFetSwitch_off' }
        websocket.send(switchVal);
    }
    function DischargeFetSwitch() {
        let switchVal;
        if (document.getElementById('disChargeFetState').checked) { switchVal = 'dischargeFetSwitch_on' }
        else { switchVal = 'dischargeFetSwitch_off' }
        websocket.send(switchVal);
    }
</script>
)rawliteral";