<script lang="ts">
    import { browser } from "$app/environment";
    import { onMount } from 'svelte';
    let webSocket = 0;
    let element = "";
    let relais = [];
    // TODO Multiple sides for timer etz 
    // TODO Support multiple esp websockets
    onMount(async () => {
        webSocket = new WebSocket("ws://192.168.178.120/ws", "protocolOne"); // TODO use Browser cookis for websocket connections
        webSocket.onopen = function(e) {
                
        };
        webSocket.onmessage = function(event) {
                //alert(`[message] Data received from server: ${event.data}`);
                let data = JSON.parse(event.data);
                console.log("data: ", data);
                switch (data.event) {
                case "relaisnumber":
                    console.log("relais nummber", data.relais)
                    for (let i = 0;i < data.relais; i++) {
                        relais.push({status: 0});
                    }
                    relais = [...relais]
                    break;
                case "relaisstates":
                    for (let status of data.data) {
                        console.log("refdsgdrtf",status.relais, status.state);
                        let variable:number = status.state;
                        relais[status.relais].status = variable;
                    }
                    break;
                case "relaisstate":
                    console.log(data.state);
                    let variable = data.state;
                    relais[data.relais].status = variable;
                    console.log("relaisstate");
                    break;
                default:
                    //alert(data.event);
                }
                console.log(relais);
            };
        websocket.onclose = function() {
            alert("Connection closed");
        };
    });
	function setRelais(number:number, bool:bool){
        console.log("sRel"+number.toString() + bool.toString());
        webSocket.send("sRel"+number.toString() + bool.toString());
    }
    function GetBit(){}
</script>

<section>
    {#each relais as object, i}
        {#if object.status}
        <button on:click={() => setRelais(i, 0)} class="on">
            Steckdose {i}
        </button>
        {:else}
        <button on:click={() => setRelais(i, 1)} class="off">
            Steckdose {i}
        </button>
        {/if}
        <br/>
    {/each}
</section>

<style>
    .on{
        border-color: transparent;
        background-color: #16A085;
    }
    .off{
        border-color: transparent;
        background-color: #C0392B;
    }
</style>