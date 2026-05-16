var hmgMaxSerialLen = 14;
var hmgMaxDevices = 32;

/**
 * Collect channel serials configured in ETS
 * @param {object} device 
 * @returns {object} serial number to list of channels
 */
function HMG_getDeviceSerials(device) {
    // collect channel serials configured in ETS
    var serialToChannel = {};
    var visibleChannels = device.getParameterByName("HMG_VisibleChannels").value;
    for (var i = 1; i <= visibleChannels; i++) {
        var channelType = device.getParameterByName("HMG_d" + i + "DeviceType").value;
        if (channelType != 0) {
            var channelSerial = device.getParameterByName("HMG_d" + i + "DeviceSerial").value;
    
            var channels = serialToChannel[channelSerial] || [];
            channels.push(i);
            serialToChannel[channelSerial] = channels;
        }
    }
    return serialToChannel;
}

/**
 * Wrapper for invoking function properties for this module Homematic
 * @param {object} online - The online connection object
 * @param {number[]} request - Request data array
 * @returns {number[]} Response data array
 */
function HMG_invokeFunctionProperty(online, request) {
    var hmgFunctionId = 160;
    var hmgPropertyId = 7;
    return online.invokeFunctionProperty(hmgFunctionId, hmgPropertyId, request);
}

/**
 * Get the number of assigned devices from CCU
 * @param {object} online - The online connection object
 * @returns {object|null} Object with found and ignored counts, or null on error
 */
function HMG_invokeDeviceCount(online) {
    // request: command=0(COUNT)
    var response = HMG_invokeFunctionProperty(online, [0]);
    // expected: [0, found_hi, found_lo, ignored]
    if (response.length >= 4 && response[0] == 0) {
        // => result is OK && at least device count available
        return {
            "found": (response[1] << 8) + response[2],
            "ignored": response[3]
        };
    } else {
        // TODO check throwing error on response format mismatch
        return null;
    }
}


/**
 * Get detail for one device from CCU
 * @param {object} online - The online connection object
 * @param {number} index - Device index (0-based)
 * @returns {object|null} Object with serial and type, or null on error
 */
function HMG_invokeDeviceDetails(online, index) {
    // request: command=1(DETAILS), index=i
    var response = HMG_invokeFunctionProperty(online, [1, index]); // TODO check using new function id
    // expected: [0, serial_1, serial_2, ..., serial_14, 0, type_1, type_2, ...]
    if (response.length >= 11 && response[0] == 0) {
        // => result is OK && at least device serial available
        var i = 1;
        var devSerial = "";
        while (response[i] > 0 && i <= hmgMaxSerialLen) {
            devSerial += String.fromCharCode(response[i++]);
        }
        var devType = "";
        while (response[i] > 0 && i < response.length) {
            devType += String.fromCharCode(response[i++]);
        }
        return {
            "serial": devSerial,
            "type": devType 
        };
    } else {
        // TODO check throwing error on response format mismatch
        return null;
    }
}

function HMG_ccuKnownDevices(device, online, progress, context) {
    var completed = false;
    var found = 0;
    var ignored = 0;
    var devList = [];

    progress.setProgress(1);
    progress.setText("Homematic: Sammle bereits konfigurierte Geräte...");
    var serialToChannel = HMG_getDeviceSerials(device);

    progress.setProgress(5);
    progress.setText("Verbinde...");
    online.connect();

    progress.setProgress(10);
    progress.setText("Homematic: Ermittle mit CCU verknüpfte Geräte...");
    var response = HMG_invokeDeviceCount(online);
    if (response) {

        found = response.found;
        ignored = response.ignored;
        var isCanceled = progress.isCanceled();
        for (var i = 0; !isCanceled && (i < found) && (i < hmgMaxDevices); i++) {
            progress.setText("Homematic: Ermittle Details für Gerät " + (i+1) + " von " + found + " ...");
            progress.setProgress(20 + (i * 80 / found));  // found > 0 guaranteed by loop condition
            try {
                var devDetails = HMG_invokeDeviceDetails(online, i);
                if (devDetails) {
                    var devSerial = devDetails.serial;
                    var devType = devDetails.type;

                    var channelList = serialToChannel[devSerial] || [];
                    var strChannel = channelList.length > 0 ? ("\tKanäle: " + channelList.join(",")) : "";
                    var strType = devType ? ("\tTyp: " + devType) : "";
                    devList.push(devSerial + strChannel + strType);
                } else {
                    // => result is not OK, skip this device
                    progress.setText("Homematic: Antwort-Fehler für Geräte-Details " + (i+1) + " von " + found);
                    devList.push("[ERRdev" + (i+1) + "]");
                }
            } catch (e) {
                progress.setText("Homematic: Fehler bei der Ermittlung der Details für Gerät " + (i+1) + " von " + found);
                devList.push("!ERRdev" + (i+1) + "!");
                Log.error("Failed getting device "+ (i+1) + ": " + e);
            }

            isCanceled = progress.isCanceled();
        }

        completed = !isCanceled;

    } else {
        // TODO check throwing error on response format mismatch
        progress.setText("Homematic: Fehler bei der Ermittlung der Geräte");
    }

    online.disconnect();

    if (completed) {
        device.getParameterByName("HMG_CcuDevices").value = devList.join("\n");

        progress.setText("Homematic: " + found + " Geräte gefunden" + (ignored > 0 ? (", " + ignored + " ignoriert") : ""));
        progress.setProgress(100);
    }
}