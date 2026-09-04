require("voice")

if not voice.IsEnabled() then
    local ok, err = voice.Enable()
    if not ok then
        print("[voice] " .. tostring(err))
        return
    end
end

local function TopTalkers(limit)
    local rows = {}
    for _, ply in ipairs(player.GetAll()) do
        local st = voice.GetStats(ply)
        if st.seconds > 0 then
            rows[#rows + 1] = { ply = ply, seconds = st.seconds }
        end
    end
    table.sort(rows, function(a, b) return a.seconds > b.seconds end)
    local out = {}
    for i = 1, math.min(limit, #rows) do
        out[i] = rows[i]
    end
    return out
end

hook.Add("PlayerSay", "voice_stats", function(ply, text)
    if text ~= "!talk" then return end
    local rows = TopTalkers(5)
    if #rows == 0 then
        ply:ChatPrint("Nobody has talked yet.")
        return ""
    end
    for i, row in ipairs(rows) do
        ply:ChatPrint(string.format("%d. %s  %s", i, row.ply:Nick(), string.NiceTime(row.seconds)))
    end
    return ""
end)

hook.Add("PlayerSay", "voice_level", function(ply, text)
    if text ~= "!mic" then return end
    for _, other in ipairs(player.GetAll()) do
        if voice.IsTalking(other) then
            ply:ChatPrint(string.format("%s is talking at %d%%", other:Nick(), voice.GetLevel(other) * 100))
        end
    end
    return ""
end)
