
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