
namespace LIBAMI {

auto Blitter::doMinterm(uint8_t operation, uint16_t a, uint16_t b, uint16_t c) const -> uint16_t {

    // taken with WolframAlpha, applied also Conjunctive normal form, Algebraic normal form and Disjunctive normal form and take best result.
    // script to generate input for WolframAlpha

//    <?php
//    for ($i = 1; $i < 256; $i++) {
//
//        $out = "";
//        if ($i & 1)			{									$out .= "(not a and not b and not c)"; }
//        if ($i & 2)			{ if ($out != "")	$out .= " or "; $out .= "(not a and not b and c)"; }
//        if ($i & 4)			{ if ($out != "")	$out .= " or "; $out .= "(not a and b and not c)"; }
//        if ($i & 8)			{ if ($out != "")	$out .= " or "; $out .= "(not a and b and c)"; }
//
//        if ($i & 16)		{ if ($out != "")	$out .= " or "; $out .= "(a and not b and not c)"; }
//        if ($i & 32)		{ if ($out != "")	$out .= " or "; $out .= "(a and not b and c)"; }
//        if ($i & 64)		{ if ($out != "")	$out .= " or "; $out .= "(a and b and not c)"; }
//        if ($i & 128)		{ if ($out != "")	$out .= " or "; $out .= "(a and b and c)"; }
//
//        echo $i . " " . $out . "\n";
//    }

    switch(operation) {
        case 1:     return ~a & ~b & ~c;
        case 2:     return ~a & ~b & c;
        case 3:     return ~(a | b);
        case 4:     return ~a & b & ~c;
        case 5:     return ~(a | c);
        case 8:     return ~a & b & c;
        case 10:    return ~a & c;
        case 12:    return ~a & b;
        case 15:    return ~a;
        case 16:    return a & ~b & ~c;
        case 17:    return ~b & ~c;
        case 32:    return a & ~b & c;
        case 34:    return ~b & c;
        case 48:    return a & ~b;
        case 51:    return ~b;
        case 60:    return a ^ b;
        case 63:    return ~(a & b);
        case 64:    return a & b & ~c;
        case 68:    return b & ~c;
        case 80:    return a & ~c;
        case 85:    return ~c;
        case 90:    return a ^ c;
        case 95:    return ~(a & c);
        case 102:   return b ^ c;
        case 105:   return ~(a ^ b ^ c);
        case 119:   return ~(b & c);
        case 127:   return ~(a & b & c);
        case 128:   return a & b & c;
        case 136:   return b & c;
        case 150:   return a ^ b ^ c;
        case 153:   return ~(b ^ c);
        case 160:   return a & c;
        case 165:   return ~(a ^ c);
        case 170:   return c;
        case 175:   return ~a | c;
        case 187:   return ~b | c;
        case 191:   return ~a | ~b | c;
        case 192:   return a & b;
        case 195:   return ~(a ^ b);
        case 204:   return b;
        case 207:   return ~a | b;
        case 221:   return b | ~c;
        case 223:   return ~a | b | ~c;
        case 238:   return b | c;
        case 239:   return ~a | b | c;
        case 240:   return a;
        case 243:   return a | ~b;
        case 245:   return a | ~c;
        case 247:   return a | ~b | ~c;
        case 250:   return a | c;
        case 251:   return a | ~b | c;
        case 252:   return a | b;
        case 253:   return a | b | ~c;
        case 254:   return a | b | c;

        case 6:     return b ^ c ^ (a & b) ^ (a & c);
        case 7:     return ~a & ~(b & c);
        case 9:     return ~a & (~b | c) & (b | ~c);
        case 11:    return ~a & (~b | c);
        case 13:    return ~a & (b | ~c);
        case 14:    return ~a & (b | c);
        case 18:    return a ^ c ^ (a & b) ^ (c & b);
        case 19:    return ~(a & c) & ~b;
        case 20:    return a ^ b ^ (a & c) ^ (b & c);
        case 21:    return ~(a & b) & ~c;
        case 22:    return a ^ b ^ c ^ (a & b & c);
        case 23:    return ~((a & (b | c)) | (b & c));
        case 24:    return a ^ (a & b) ^ (a & c) ^ (b & c);
        case 25:    return ~(b ^ c ^ (a & b & c));
        case 26:    return a ^ c ^ (a & b) ^ (a & b & c);
        case 27:    return ~(b ^ (a & c) ^ (b & c));
        case 28:    return a ^ b ^ (a & c) ^ (a & b & c);
        case 29:    return ~(c ^ (a & b) ^ (b & c));
        case 30:    return a ^ b ^ c ^ (b & c);
        case 31:    return ~a | (~b & ~c);
        case 33:    return (~a | c) & (a | ~c) & ~b;
        case 35:    return (~a | c) & ~b;
        case 36:    return b ^ (a & b) ^ (a & c) ^ (b & c);
        case 37:    return ~(a ^ c ^ (a & b & c));
        case 38:    return b ^ c ^ (a & b) ^ (a & b & c);
        case 39:    return ~(a ^ (a & c) ^ (b & c));
        case 40:    return (a & c) ^ (b & c);
        case 41:    return ~(a ^ b ^ c ^ (a & b) ^ (a & b & c));
        case 42:    return c ^ (a & b & c);
        case 43:    return (~a & (~b | c)) | (~b & c);
        case 44:    return (a & ~b & c) | (~a & b);
        case 45:    return ~(a ^ c ^ (b & c));
        case 46:    return (~a & b) | (~b & c);
        case 47:    return ~a | (~b & c);
        case 49:    return (a | ~c) & ~b;
        case 50:    return (a | c) & ~b;
        case 52:    return a ^ b ^ (b & c) ^ (a & b & c);
        case 53:    return ~(c ^ (a & b) ^ (a & c));
        case 54:    return a ^ b ^ c ^ (a & c);
        case 55:    return (~a & ~c) | ~b;
        case 56:    return (a & ~b) | (~a & b & c);
        case 57:    return ~(b ^ c ^ (a & c));
        case 58:    return (a & ~b) | (~a & c);
        case 59:    return (~a & c) | ~b;
        case 61:    return ~(a & b) & (a | b | ~c);
        case 62:    return ~(a & b) & (a | b | c);
        case 65:    return (~a | b) & (a | ~b) & ~c;
        case 66:    return c ^ (a & b) ^ (a & c) ^ (b & c);
        case 67:    return ~(a ^ b ^ (a & b & c));
        case 69:    return (~a | b) & ~c;
        case 70:    return b ^ c ^ (a & c) ^ (a & b & c);
        case 71:    return ~(a ^ (a & b) ^ (b & c));
        case 72:    return (a & b) ^ (c & b);
        case 73:    return ~(a ^ b ^ c ^ (a & c) ^ (a & b & c));
        case 74:    return (a & b & ~c) | (~a & c);
        case 75:    return ~(a ^ b ^ (b & c));
        case 76:    return b ^ (a & b & c);
        case 77:    return (~a & (b | ~c)) | (b & ~c);
        case 78:    return b ^ c ^ (a & c) ^ (b & c);
        case 79:    return ~a | (b & ~c);
        case 81:    return (a | ~b) & ~c;
        case 82:    return a ^ c ^ (c & b) ^ (a & c & b);
        case 83:    return ~(b ^ (a & b) ^ (a & c));
        case 84:    return (a | b) & ~c;
        case 86:    return a ^ b ^ c ^ (a & b);
        case 87:    return (~a & ~b) | ~c;
        case 88:    return (a & ~c) | (~a & b & c);
        case 89:    return ~(b ^ c ^ (a & b));
        case 91:    return ~(a & c) & (a | ~b | c);
        case 92:    return (a & ~c) | (~a & b);
        case 93:    return (~a & b) | ~c;
        case 94:    return ~(a & c) & (a | b | c);
        case 96:    return (a & b) ^ (a & c);
        case 97:    return ~(a ^ b ^ c ^ (b & c) ^ (a & b & c));
        case 98:    return (a & b & ~c) | (~b & c);
        case 99:    return ~(a ^ b ^ (a & c));
        case 100:   return (a & ~b & c) | (b & ~c);
        case 101:   return ~(a ^ c ^ (a & b));
        case 103:   return (~a | b | c) & ~(b & c);
        case 104:   return (a & b) ^ (a & c) ^ (b & c) ^ (a & b & c);
        case 106:   return c ^ (a & b);
        case 107:   return ~(a ^ b ^ (a & c) ^ (b & c) ^ (a & b & c));
        case 108:   return b ^ (a & c);
        case 109:   return ~(a ^ c ^ (a & b) ^ (b & c) ^ (a & b & c));
        case 110:   return b ^ c ^ (b & c) ^ (a & b & c);
        case 111:   return ~(a ^ (a & b) ^ (a & c));
        case 112:   return a ^ (a & b & c);
        case 113:   return ~((~a & (b | c)) | (b & c));
        case 114:   return a ^ c ^ (a & c) ^ (b & c);
        case 115:   return (a & ~c) | ~b;
        case 116:   return (a & ~b) | (b & ~c);
        case 117:   return (a & ~b) | ~c;
        case 118:   return (a | b | c) & ~(b & c);
        case 120:   return a ^ (b & c);
        case 121:   return ~(b ^ c ^ (a & b) ^ (a & c) ^ (a & b & c));
        case 122:   return a ^ c ^ (a & c) ^ (a & b & c);
        case 123:   return ~(b ^ (a & b) ^ (c & b));
        case 124:   return a ^ b ^ (a & b) ^ (a & b & c);
        case 125:   return ~(c ^ (a & c) ^ (b & c));
        case 126:   return ~(a & b & c) & (a | b | c);
        case 129:   return (~a & ~b & ~c) | (a & b & c);
        case 130:   return c ^ (a & c) ^ (b & c);
        case 131:   return (a & b & c) | (~a & ~b);
        case 132:   return b ^ (a & b) ^ (c & b);
        case 133:   return (a & b & c) | (~a & ~c);
        case 134:   return b ^ c ^ (a & b) ^ (a & c) ^ (a & b & c);
        case 135:   return ~(a ^ (b & c));
        case 137:   return (~a & ~b & ~c) | (b & c);
        case 138:   return (~a | b) & c;
        case 139:   return (~a & ~b) | (b & c);
        case 140:   return (~a | c) & b;
        case 141:   return (~a & ~c) | (b & c);
        case 142:   return (~a & (b | c)) | (b & c);
        case 143:   return ~a | (b & c);
        case 144:   return a ^ (a & b) ^ (a & c);
        case 145:   return (a & b & c) | (~b & ~c);
        case 146:   return a ^ c ^ (a & b) ^ (b & c) ^ (a & b & c);
        case 147:   return ~(b ^ (a & c));
        case 148:   return a ^ b ^ (a & c) ^ (b & c) ^ (a & b & c);
        case 149:   return ~(c ^ (a & b));
        case 151:   return ~((a & b) ^ (a & c) ^ (b & c) ^ (a & b & c));
        case 152:   return (a & ~b & ~c) | (b & c);
        case 154:   return a ^ c ^ (a & b);
        case 155:   return (~a | b | ~c) & (~b | c);
        case 156:   return a ^ b ^ (a & c);
        case 157:   return (~a | ~b | c) & (b | ~c);
        case 158:   return a ^ b ^ c ^ (b & c) ^ (a & b & c);
        case 159:   return ~((a & b) ^ (a & c));
        case 161:   return (a & c) | (~a & ~b & ~c);
        case 162:   return (a | ~b) & c;
        case 163:   return (a & c) | (~a & ~b);
        case 164:   return (a & c) | (~a & b & ~c);
        case 166:   return b ^ c ^ (a & b);
        case 167:   return (~a | c) & (a | ~b | ~c);
        case 168:   return (a | b) & c;
        case 169:   return ~(a ^ b ^ c ^ (a & b));
        case 171:   return (~a & ~b) | c;
        case 172:   return (a & c) | (~a & b);
        case 173:   return (~a | c) & (a | b | ~c);
        case 174:   return (~a & b) | c;
        case 176:   return a & (~b | c);
        case 177:   return (a & c) | (~b & ~c);
        case 178:   return (a & (~b | c)) | (~b & c);
        case 179:   return (a & c) | ~b;
        case 180:   return a ^ b ^ (b & c);
        case 181:   return (~a | ~b | c) & (a | ~c);
        case 182:   return a ^ b ^ c ^ (a & c) ^ (a & b & c);
        case 183:   return ~((a & b) ^ (c & b));
        case 184:   return (a & ~b) | (b & c);
        case 185:   return (a | b | ~c) & (~b | c);
        case 186:   return (a & ~b) | c;
        case 188:   return a ^ b ^ (a & b & c);
        case 189:   return (~a | ~b | c) & (a | b | ~c);
        case 190:   return (a & ~b) | (~a & b) | c;
        case 193:   return (a & b) | (~a & ~b & ~c);
        case 194:   return (a & b) | (~a & ~b & c);
        case 196:   return (a | ~c) & b;
        case 197:   return (a & b) | (~a & ~c);
        case 198:   return b ^ c ^ (a & c);
        case 199:   return (~a | b) & (a | ~b | ~c);
        case 200:   return (a | c) & b;
        case 201:   return ~(a ^ b ^ c ^ (a & c));
        case 202:   return (a & b) | (~a & c);
        case 203:   return (~a | b) & (a | ~b | c);
        case 205:   return (~a & ~c) | b;
        case 206:   return (~a & c) | b;
        case 208:   return a & (b | ~c);
        case 209:   return (a & b) | (~b & ~c);
        case 210:   return a ^ c ^ (b & c);
        case 211:   return (~a | b | ~c) & (a | ~b);
        case 212:   return (a & (b | ~c)) | (b & ~c);
        case 213:   return (a & b) | ~c;
        case 214:   return a ^ b ^ c ^ (a & b) ^ (a & b & c);
        case 215:   return ~((a & c) ^ (b & c));
        case 216:   return a ^ (a & c) ^ (b & c);
        case 217:   return (a | ~b | c) & (b | ~c);
        case 218:   return a ^ c ^ (a & b & c);
        case 219:   return (~a | b | ~c) & (a | ~b | c);
        case 220:   return (a & ~c) | b;
        case 222:   return (a & ~c) | (~a & c) | b;
        case 224:   return a & (b | c);
        case 225:   return ~(a ^ b ^ c ^ (b & c));
        case 226:   return (a & b) | (~b & c);
        case 227:   return (~a | b | c) & (a | ~b);
        case 228:   return b ^ (a & c) ^ (b & c);
        case 229:   return (~a | b | c) & (a | ~c);
        case 230:   return b ^ c ^ (a & b & c);
        case 231:   return (~a | b | c) & (a | ~b | ~c);
        case 232:   return (a & (b | c)) | (b & c);
        case 233:   return ~(a ^ b ^ c ^ (a & b & c));
        case 234:   return (a & b) | c;
        case 235:   return (a & b) | (~a & ~b) | c;
        case 236:   return (a & c) | b;
        case 237:   return (a & c) | (~a & ~c) | b;
        case 241:   return a | (~b & ~c);
        case 242:   return a | (~b & c);
        case 244:   return a | (b & ~c);
        case 246:   return a | (b & ~c) | (~b & c);
        case 248:   return a | (b & c);
        case 249:   return a | (b & c) | (~b & ~c);

        case 0:     return 0;
        case 255:   return 0xffff;
    }

    _unreachable
}

}
