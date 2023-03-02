<script lang="ts">
    // @ts-nocheck
    import { onMount } from 'svelte';
    let connections = "192.168.178.122";
    let webSocket = 0;
    let relays:object = [];
    
    let timer_relay = 0;
    let timer_time = 10;
    let timer_turn = 1;
    let timer_run = 1;
    let timers:object = [];
    onMount(async () => {

	});
    function connect() {
        let connectionstring = "ws://" + connections + "/ws" 
        webSocket = new WebSocket(connectionstring, "protocolOne");
        webSocket.onopen = function(event: any) {
            webSocket.send(JSON.stringify(
                {eventtype: "getRelaysState"}
            ));
            webSocket.send(JSON.stringify(
                {eventtype: "getTimers"}
            ));
        };

        webSocket.onmessage = function(event: any) {
                //alert(`[message] Data received from server: ${event.data}`);
                let data: object = JSON.parse(event.data);
                console.log("data: ", data);
                switch (data.eventtype) {
                    case "setRelayState":
                        relays[data.Relay] = data;
                        return;
                    case "Timer":
                        //{"eventtype": "newTimer","time": 2,"Relay": 0,"turn": true,"run": true}
                        timers[data.timerID] = data;
                        return;
                    case "endTimer":
                        timers.splice(data.timerID, 1);
                        timers = [...timers];
                        return;
                    case "Tick":
                        for (let i = 0; i < timers.length; i++) {
                            if (!timers[i].run)
                                continue;
                            timers[i].time = timers[i].time-1;
                        }
                        timers = [...timers];
                        return;
                }
        };
        webSocket.onclose = function() {
            webSocket = 0;
        };
    }
    function disconnect() {
        webSocket.close();
    }
    
    function setRelais(Relay:number, state:bool) {
        webSocket.send(JSON.stringify(
            {eventtype: "setRelayState", Relay: Relay, turn:state}
            ));
    }
    function setTimer() {
        console.log(timer_relay);
        webSocket.send(JSON.stringify(
            {
                "eventtype":"setTimer",
                "time":timer_time,
                "Relay": timer_relay,
                "turn": timer_turn,
                "run": timer_run
            }
            ));
    }

    function stopTimer() {

    }
</script>

<section>
    <input value={connections}>
    {#if !webSocket}
        <button on:click={connect}>Verbinden</button>
    {:else}
        <button on:click={disconnect}>Verbindung Trennen</button>
    {/if}
    <br/>
    <h1>Relais State</h1>
    {#each relays as { Relay, turn }, i}
        <span>Relais: {Relay + 1}</span>
        <button on:click={() => setRelais(Relay, !turn)}
            class="{turn ? "on": "off"}"
            >{turn}</button>
        <br/>
    {/each}
    <h1>Timer</h1>
    <span>Relais</span>
    <input bind:value={timer_relay}>
    <span>Zeit in Sekunden</span>
    <input bind:value={timer_time}>
    <span>An/Ausschalten</span>
    <input bind:value={timer_turn}>
    <span>Start/Stop</span>
    <input bind:value={timer_run}>
    <button on:click={setTimer}>Submit</button>
    <br/>
    {#each timers as { Relay, turn, run, time}, i}
        <span> {i} Relais: {Relay + 1}</span>
        <span>{time}</span>
        <span>Turn:</span>
        <span class="{turn ? "on": "off"}">{turn ? "on": "off"}</span>
        <button on:click={stopTimer}
            class="{run ? "on": "off"}"
            >{run}</button>
        <span>🗑️</span>
        <br/>
    {/each}
</section>

<style>
    button {
        border: none;
        margin: 2px;
    }
    .on {
        background-color: #117864;
    }
    .off {
        background-color: #A93226;
    }
</style>