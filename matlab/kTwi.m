% 
%       __            ____
%      / /__ _  __   / __/                      __  
%     / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
%    / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
%   /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
%                     /_/   github.com/KitSprout    
%  
%  @file    kTwi.m
%  @author  KitSprout
%  @date    Dec-2019
%  @brief   
% 

classdef kTwi < handle

properties (SetAccess = private)
    s;
end

methods

    function twi = kTwi( varargin )
        switch nargin
            case 1
                twi.s = varargin{1};
            otherwise
                error('input error!!');
        end
    end

    function delay( twi, second )
        twi.s.delay(second);
    end

    % [rd, cnt] = twi.read(address, register, lenght)
    function varargout = read( twi, address, register, lenght, timeout )
        if (nargin < 5)
            timeout = 1000;
        end
        twi.s.packetSend(uint8([address*2+1, register]), lenght, 'R1');

        count = 0;
        ri = [];
        while isempty(ri) && count < timeout
            [ri, rd] = twi.s.packetRecv();
            count = count + 1;
        end

        if count >= timeout
            count = -1;
        end

        varargout = { rd, count };
    end

    % [wi, wb] = twi.write(address, register, data)
    function varargout = write( twi, address, register, data )
        [info, send] = twi.s.packetSend(uint8([address*2, register]), data, 'R1');
        varargout = { info, send };
    end

    % address = twi.scandevice()
    % address = twi.scandevice('printon')
    function varargout = scandevice( twi, varargin )
        [ri, rd] = twi.s.packetSendRecv([171, 0], 0, 'R2');

        if strcmp('printon', varargin{end})
            fprintf('\n');
            fprintf(' >> i2c device list (found %d device)\n\n', size(rd, 1));
            fprintf('    ');
            for i = 1 : size(rd, 1)
                fprintf(' %02X', rd(i));
            end
            fprintf('\n\n');
        end
        
        varargout = { rd, ri };
    end

    % reg = twi.scanregister(address)
    % reg = twi.scanregister(address, 'printon')
    function varargout = scanregister( twi, address, varargin )
        [ri, rd] = twi.s.packetSendRecv([203, address*2], 0, 'R2');

        if strcmp('printon', varargin{end})
            fprintf('\n');
            fprintf(' >> i2c device register (address %02X)\n\n', address);
            fprintf('      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n');
            for i = 1 : 16 :256
                fprintf(' %02X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n', ...
                i-1, rd(i:i+16-1));
            end
            fprintf('\n');
        end

        varargout = { rd, ri };
    end

end

methods (Access = private)

end

end
