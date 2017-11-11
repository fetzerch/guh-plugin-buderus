# buderus-wireshark

Wireshark dissector that displays Buderus KM200 packets transported over
HTTP or XMPP.

## Dependencies and Installation

This dissector uses [luagcrypt](https://github.com/Lekensteyn/luagcrypt/)
(licensed under MIT), a Lua wrapper for
[Libgcrypt](https://gnupg.org/software/libgcrypt/). See
<https://github.com/Lekensteyn/luagcrypt#installation> for installation
instructions.

Install this plugin by copying the content of the directory to
`~/.config/wireshark/plugins`.

This code uses (and ships) [json.lua](https://github.com/rxi/json.lua)
(licensed under MIT).

## Usage

The dissector is automatically enabled for HTTP and XMPP packages.
The decryption key(s) have to be configured in the Wireshark Protocol
Preference dialog for `BUDERUS_HTTP` and `BUDERUS_XMPP`.
