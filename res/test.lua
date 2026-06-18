local Test = {}

function Test:Yay()
    debug_log("Yielding for one tick...")
    coroutine.yield()
    debug_log("Resumed after one tick.")

    debug_log("Waiting 1 second...")
    wait_seconds(1.0)
    debug_log("Done waiting.")

    parallelize(
        function()
            wait_seconds(0.5)
            debug_log("Parallel task A finished (0.5s)")
        end,
        function()
            wait_seconds(1.0)
            debug_log("Parallel task B finished (1.0s)")
        end,
        function()
            wait_seconds(0.25)
            debug_log("Parallel task C finished (0.25s)")
        end
    )

    debug_log("waiting for signal...")
    wait_signal("test_ready")
    debug_log("yay it works")
end

return Test
