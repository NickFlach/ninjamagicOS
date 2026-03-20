// hello_agent.sp — Example Shinobi.Substrate cognitive program
//
// A minimal NinjaMagic Agent that subscribes to phone events
// and stores them in associative memory.
//
// Compile: spc hello_agent.sp -o hello_agent.rs

domain HelloAgent {
    grant events "phone/"
    grant events "agent/"
    grant assoc "working" rw
    grant clock
    sealed
}

lane main in HelloAgent {
    priority high
    energy balanced
    affinity big

    event.publish("agent/status", "hello_agent ready")

    loop {
        let ev = event.wait("phone/", 1s)
        assoc.put("working", "last_event", ev.topic)
        event.publish("agent/echo", ev.payload)
    }
}
