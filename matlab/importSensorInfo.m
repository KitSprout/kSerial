
sv = struct;
sv.t_unit = 0.001;
sv.g_sens = 16.4;   % dps
sv.a_sens = 16384;  % g
sv.m_sens = 6.6;    % uT
sv.t_idx = 1 : 2;
sv.g_idx = sv.t_idx(end) + (1 : 3);
sv.a_idx = sv.g_idx(end) + (1 : 3);
sv.m_idx = sv.a_idx(end) + (1 : 3);
