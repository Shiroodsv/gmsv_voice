require("voice")

voice.SetBufferSeconds(60)

local ok, err = voice.Enable()
if not ok then
    print("[voice] " .. tostring(err))
    return
end

local function FindPlayer(text)
    text = string.lower(text)
    for _, ply in ipairs(player.GetAll()) do
        if string.find(string.lower(ply:Nick()), text, 1, true) then
            return ply
        end
    end
end

hook.Add("PlayerSay", "voice_clips", function(ply, text)
    local args = string.Explode(" ", text)
    if args[1] ~= "!clip" or not ply:IsAdmin() then return end

    local target = FindPlayer(args[2] or "")
    if not target then
        ply:ChatPrint("No player matched.")
        return ""
    end

    local seconds = math.Clamp(tonumber(args[3]) or 30, 1, voice.GetBufferSeconds())
    local name = string.format("clips/%s_%s.wav", target:SteamID64(), os.date("%Y%m%d_%H%M%S"))
    file.CreateDir("clips")

    local saved, reason = voice.SaveWav(target, seconds, name)
    if saved then
        ply:ChatPrint(string.format("Saved %ds of %s to data/%s", seconds, target:Nick(), name))
    else
        ply:ChatPrint("Clip failed: " .. tostring(reason))
    end
    return ""
end)
