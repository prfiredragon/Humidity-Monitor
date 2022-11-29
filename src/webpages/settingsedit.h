const char HTML_SETTINGS_EDIT[] PROGMEM = R"rawliteral(
<figure class="text-center">
    <h1>Edit Configuration</h1>
</figure>
<form method="POST" action="/settingssave" enctype="multipart/form-data">
    <div class="input-group mb-3">
        <span class="input-group-text w-50" id="devicenamedesc">Device Name</span>
        <input type="text" class="form-control" aria-describedby="devicenamedesc" id="devicename" name="post_deviceName"
            value="">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttserverdesc">MQTT Server</span>
        <input type="text" class="form-control" aria-describedby="mqttserverdesc" id="mqttserver" name="post_mqttServer"
            value="">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttportdesc">MQTT Port</span>
        <input type="text" class="form-control" aria-describedby="mqttportdesc" id="mqttport" name="post_mqttPort"
            value="">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttuserdesc">MQTT User</span>
        <input type="text" class="form-control" aria-describedby="mqttuserdesc" id="mqttuser" name="post_mqttUser"
            value="">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttpassworddesc">MQTT Password</span>
        <input type="text" class="form-control" aria-describedby="mqttpassworddesc" id="mqttpassword"
            name="post_mqttPassword" value="">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqtttopicdesc">MQTT Topic</span>
        <input type="text" class="form-control" aria-describedby="mqtttopicdesc" id="mqtttopic" name="post_mqttTopic"
            value="">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttrefreshdesc">MQTT Refresh</span>
        <input type="text" class="form-control" aria-describedby="mqttrefreshdesc" id="mqttrefresh"
            name="post_mqttRefresh" value="">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttjsondesc">MQTT Json Style</span>
        <div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="mqttjsondesc"
                role="switch" id="mqttjson" name="post_mqttjson" value="true" />
        </div>
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="default_wet_valdesc">wet val</span>
        <input type="text" class="form-control" aria-describedby="default_wet_valdesc" id="default_wet_val"
            name="post_default_wet_val" value="">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="default_dry_valdesc">dry val</span>
        <input type="text" class="form-control" aria-describedby="default_dry_valdesc" id="default_dry_val"
            name="post_default_dry_val" value="">
    </div>

    <div class="d-grid gap-2">
        <input class="btn btn-primary" type="submit" value="Save settings">
</form>
<a class="btn btn-primary" href="/settings" role="button">Back</a>
</div>
<script>
    $(document).ready(function (load) {
        $.ajax({
            url: "settingsjson",
            data: {},
            type: "get",
            dataType: "json",
            cache: false,
            success: function (data) {
                document.getElementById("devicename").value = data.device_name;
                document.getElementById("mqttserver").value = data.mqtt_server;
                document.getElementById("mqttport").value = data.mqtt_port;
                document.getElementById("mqtttopic").value = data.mqtt_topic;
                document.getElementById("mqttuser").value = data.mqtt_user;
                document.getElementById("mqttpassword").value = data.mqtt_password;
                document.getElementById("mqttrefresh").value = data.mqtt_refresh;
                document.getElementById("mqttjson").checked = data.mqtt_json;
                document.getElementById("default_wet_val").value = data.default_wet_val;
                document.getElementById("default_dry_val").value = data.default_dry_val;
            }
        });
    });
</script>
)rawliteral";