local curl   = require "lcurl";
local decode = require "json.decode";

local url_sms_cdr  = 'http://172.16.0.60/json/SMScdr.php?';

function hpvp_cdr(uuid,clg,cld)
    local url_str = url_sms_cdr..'uuid="'..uuid..'"&clg='..clg..'&cld='..cld;

    curl.easy()
	:setopt_url(url_str)
	:setopt_writefunction(function(str) JSONString=str end)
	:perform()
	:close()

    return decode.decode(JSONString);
end

json_result = hpvp_cdr('1111','359996666199','359882079235');
print(json_result['Status']);