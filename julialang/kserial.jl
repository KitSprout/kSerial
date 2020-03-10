#        __            ____
#       / /__ _  __   / __/                      __  
#      / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
#     / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
#    /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
#                      /_/   github.com/KitSprout    
#   
#   @file    kSerial.py
#   @author  KitSprout
#   @date    Mar-2020
#   @brief   kSerial packet format :
#            byte 1   : header 'K' (75)       [HK]
#            byte 2   : header 'S' (83)       [HS]
#            byte 3   : data bytes (12-bit)   [L ]
#            byte 4   : data type             [T ]
#            byte 5   : parameter 1           [P1]
#            byte 6   : parameter 2           [P2]
#            byte 7   : checksum              [CK]
#             ...
#            byte L-1 : data                  [DN]
#            byte L   : finish '\r' (13)      [ER]

using LibSerialPort
using Printf

typeconv = Dict(
    # string to value
    "UInt8"   => 0, "UInt16"  =>  1, "UInt32"  =>  2, "UInt64" =>  3,
    "Int8"    => 4, "Int16"   =>  5, "Int32"   =>  6, "Int64"  =>  7,
    "Float16" => 9, "Float32" => 10, "Float64" => 11,
    "R0"      => 8, "R1"      => 12, "R2"      => 13, "R3"     => 14, "R4" => 15,
    # value to string
    0 => "UInt8",    1 => "UInt16",   2 => "UInt32",   3 => "UInt64",
    4 => "Int8",     5 => "Int16",    6 => "Int32",    7 => "Int64",
    9 => "Float16", 10 => "Float32", 11 => "Float64",
    8 => "R0",      12 => "R1",      13 => "R2",      14 => "R3",     15 => "R4"
);

# packet = pack();                   ->  R0 (8-bit)                    -> [75, 83, 0, 8, 0, 0, 8, 13]
# packet = pack(param);              ->  only send parameter (8-bit)   -> [75, 83, 0, 8, P1, P2, CK, 13]
# packet = pack(param, type);        ->  user define data type (8-bit) -> [75, 83, L, T, P1, P2, CK, 13]
# packet = pack(param, data);        ->  automatic detect data type    -> [75, 83, L, T, P1, P2, Dn, CK, 13]
# packet = pack(param, data, type);  ->  user define data type         -> [75, 83, L, T, P1, P2, Dn, CK, 13]
function pack(param::Array{UInt8,1}=UInt8[0,0], type::Union{Integer,String}="R0", data::Vector=[])
    # @printf("[DBG] param = %s, type = %s, data = %s\n", typeof(param), typeof(type), typeof(data));
    # @printf("[DBG] param = %d, type = %d, data = %d, size = %d\n", length(param), length(type), length(data), sizeof(data));

    packet_bytes = sizeof(data);
    packet_type  = (packet_bytes >> 8) << 4;
    try
        # check type input
        typeconv[type];
        if typeof(type) == String
            packet_type += typeconv[type];
        else
            packet_type += type;
        end
    catch e
        @printf("can't find dict key-value: %s\n", e);
    end

    if length(data) == 0
        pdata = UInt8[];
    else
        try
            pdata = reinterpret(UInt8, data);
        catch e
            @printf("reinterpret error: %s\n", e);
        end
    end

    packet = UInt8['K', 'S'];               # header 'KS'
    append!(packet, packet_bytes % 256);    # data bytes
    append!(packet, packet_type);           # data type
    append!(packet, param);                 # parameter
    checksum = (packet[3] + packet[4] + packet[5] + packet[6]) % 256;
    append!(packet, checksum);              # checksum
    append!(packet, pdata);                 # data
    append!(packet, '\r');                  # finish '\r'

    return packet;
end

# info, data, last_index = unpack(packet);
# info[:, 1] : data length (bytes)
# info[:, 2] : data type
# info[:, 3] : parameter 1
# info[:, 4] : parameter 2
# info[:, 5] : checksum
function unpack(packet::Array{UInt8,1})
    # @printf("[DBG] packet = %s, %d\n", typeof(packet), length(packet));
    # println(packet);

    info = [];
    data = [];
    last_index = 0;

    packet_bytes = length(packet);

    # check 'KS'
    Ki = findall(x->x == UInt8('K'), packet);
    Ki = Ki[findall(x->x <= packet_bytes, Ki .+ 7)];
    Ki = Ki[findall(x->x == UInt8('S'), packet[Ki .+ 1])];
    if length(Ki) != 0
        # get packet info and checksum
        Ld  = packet[Ki .+ 2];
        Td  = packet[Ki .+ 3];
        P1d = packet[Ki .+ 4];
        P2d = packet[Ki .+ 5];
        CKd = packet[Ki .+ 6];
        # check checksum
        checksum = Ld + Td + P1d + P2d;
        Ki = Ki[findall(x->x == true, CKd .== checksum)];
        Ld = packet[Ki .+ 2];
        Td = packet[Ki .+ 3];
        # get packet data lens
        packet_lens = Ld + (UInt16.(Td) .>> 4) .<< 8;
        # check last packet
        idx = findall(x->x <= packet_bytes, Ki .+ packet_lens .+ 7);
        Er = Ki .+ packet_lens .+ 7;
        # check '\r'
        idx = findall(x->x == UInt8('\r'), packet[Er[idx]]);
        Ki = Ki[idx];
        if length(Ki) != 0
            # get info
            packet_lens  = packet_lens[idx];
            packet_type  = packet[Ki .+ 3] .& 0x0F;
            packet_param = [packet[Ki .+ 4], packet[Ki .+ 5]];
            packet_param = [packet[Ki .+ 4], packet[Ki .+ 5]];
            # info = Int.(hcat([packet_lens, packet_type, packet[Ki .+ 4], packet[Ki .+ 5], packet[Ki .+ 6]]...)'); # 5xN
            info = Int.(hcat([packet_lens, packet_type, packet[Ki .+ 4], packet[Ki .+ 5], packet[Ki .+ 6]]...));    # Nx5
            # get data
            data = [];
            for i = 1 : length(Ki)
                append!(data, [packet[Ki[i] + 6 .+ (1:packet_lens[i])]]);
            end
        end
    end

    return info, data, last_index;
end

function send(s, param, type, data)
    pkdata = pack(param, type, data);
    write(s, pkdata);
    return pkdata, length(pkdata);
end

function recv(s)
    nb = bytesavailable(s);
    packet = readbytes!(s, nb);
    info, data, last_index = unpack(packet);
    return info, data, nb;
end

# pk(UInt8[0x01, 0x00], "R1", [])
function pk(param, type, data)
    println(param, ", ", type, ", ", data);

    packet = pack(param, type, data);
    info, data, last_index = unpack(packet);
    println(UInt8.(info[:][3:4]), ", ", typeconv[info[:][2]], ", ", data[1][:]);
end

function check(delay)
    send(sp, UInt8[0x00, 0x00], "R2", []);
    sleep(delay);
    info, data, nb = recv(sp);
    return data;
end

function scandevice(delay)
    send(sp, UInt8[0xAB, 0x00], "R2", []);
    sleep(delay);
    info, data, nb = recv(sp);
    return data;
end

function scanreg(address::UInt8, delay)
    send(sp, UInt8[0xCB, (address << 1) + 1], "R2", []);
    sleep(delay);
    info, data, nb = recv(sp);
    return data;
end

sp = open("COM3", 115200);
println(check(0.01));
println(scandevice(0.01));
println(scanreg(0x0c, 0.1));
close(sp);
