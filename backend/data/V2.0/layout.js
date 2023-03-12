//var gateway = `ws://${window.location.hostname}/ws`;
var gateway = "ws://192.168.178.123/ws"
webSocket = new WebSocket(gateway, "protocolOne");
webSocket.onopen = function(event) {};
webSocket.onmessage = function(event) {
  let data = JSON.parse(event.data);
  console.log("data: ", data);
  switch (data.eventtype) {
  case "Tick":
    const timers = document.getElementsByClassName("tick");
    for (let i = 0; i < timers.length; i++) {
        let str = parseInt(timers[i].dataset.time ) - 1;
        if (timers[i].dataset.time > -1 && timers[i].dataset.edit != false && timers[i].dataset.timerrun == "true") {
            timers[i].dataset.time = str.toString();
            displayTime(document.getElementById("timer".concat(i)), str);
        }
    }
    return;
  case "setRelayState":
    document.getElementById("state".concat(data.relay)).dataset.state = data.state ? "ON":"OFF";
    return;
  case "setTimer":
    document.getElementById("timer".concat(data.timerID)).dataset.time = data.time +1;
    document.getElementById("timer".concat(data.timerID)).dataset.timerrun = data.state;
    document.getElementById("timer".concat(data.timerID)).dataset.timerstate = data.turn;
    displayTime(document.getElementById("timer".concat(data.timerID)), data.time);
    return;
  case "stopTimer":
    document.getElementById("timer".concat(data.timerID)).dataset.timerrun = data.state;
    return;
  case "turnTimer":
    document.getElementById("timer".concat(data.timerID)).dataset.timerstate = data.turn;
    return;
  }
  
}

function displayTime(timerObject, timeInSec){
    if (timerObject.dataset.edit == "true") {return;}
    if (isNaN(timeInSec)){return;}
    if (timeInSec <= 0){
        timerObject.querySelector('.h').value = "";
        timerObject.querySelector('.min').value = "";
        timerObject.querySelector('.s').value = "";
        return;
    }
    timerObject.querySelector('.h').value = Math.floor(timeInSec/3600);
    timerObject.querySelector('.min').value = Math.floor((timeInSec%3600)/60);
    timerObject.querySelector('.s').value = timeInSec%60;
}

function ToggleRelay(relay, relayClass){
  webSocket.send(JSON.stringify({
              eventtype: "setRelayState",
              relay: relay,
              state: document.getElementById(relayClass).dataset.state == "ON" ? 0 : 1
              }));
}

function stopTimer(timerID, relayClass){
    if(document.getElementById(relayClass).dataset.edit == "false"){
    webSocket.send(JSON.stringify({
        eventtype: "stopTimer",
        timerID: timerID,
        state: document.getElementById(relayClass).dataset.timerrun == "false" ? true : false
        }));
        return;
    }
    document.getElementById(relayClass).dataset.editrun = document.getElementById(relayClass).dataset.editrun == "false" ? "true": "false";
}

function edit(state, relayClass){
    document.getElementById(relayClass).dataset.edit = state;
    document.getElementById(relayClass).dataset.editrun = document.getElementById(relayClass).dataset.timerrun;
    document.getElementById(relayClass).dataset.editturn = document.getElementById(relayClass).dataset.timerstate == "false" ? "true": "false";
}

function send(timerID,relayClass){
    let timerObject = document.getElementById(relayClass);
    let sec = parseInt((timerObject.querySelector('.h').value) || 0) * 3600 +
        (parseInt(timerObject.querySelector('.min').value) || 0) * 60 +
        (parseInt(timerObject.querySelector('.s').value) || 0);
    if (document.getElementById(relayClass).dataset.edit == "false"){
        webSocket.send(JSON.stringify({
            eventtype: "setTimer",
            time: sec,
            timerID: timerID,
            state: document.getElementById(relayClass).dataset.timerrun == "false" ? false : true,
            turn: document.getElementById(relayClass).dataset.timerstate == "false" ? false : true
            }));
            return;
    }
    webSocket.send(JSON.stringify({
        eventtype: "setTimer",
        time: sec,
        timerID: timerID,
        state: document.getElementById(relayClass).dataset.editrun == "false" ? false : true,
        turn: document.getElementById(relayClass).dataset.editturn == "false" ? true : false
    }));
    document.getElementById(relayClass).dataset.edit = "false"
}

function timerturn(relay,relayClass){
    if(document.getElementById(relayClass).dataset.edit == "false"){
        document.getElementById(relayClass).dataset.timerstate = document.getElementById(relayClass).dataset.timerstate == "false" ? "true": "false";
        webSocket.send(JSON.stringify({
            eventtype: "turnTimer",
            timerID: relay,
            turn: document.getElementById(relayClass).dataset.timerstate == "false" ? false : true
            }));
        return;
    }
    document.getElementById(relayClass).dataset.editturn = document.getElementById(relayClass).dataset.editturn == "false" ? "true": "false";
}