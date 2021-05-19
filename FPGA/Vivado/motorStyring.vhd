----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 04.05.2021 13:18:16
-- Design Name: 
-- Module Name: motorStyring - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity motorStyring is

    generic (
        g_init_val : std_logic := '0'
    );

    Port ( clock : in STD_LOGIC;
           reset : in STD_LOGIC:= g_init_val;
           direction : in STD_LOGIC := g_init_val;
           motorSignal : out STD_LOGIC_VECTOR (3 downto 0):= (others => g_init_val) ;
           runReqIn : in STD_LOGIC:= g_init_val;
           motorRunningOut : out STD_LOGIC:= g_init_val
           );
end motorStyring;

architecture Behavioral of motorStyring is
    type motorState is (await, faseA, faseB, faseC, faseD);
	signal state_current, state_next: motorState;
	signal motorSignal_I: std_logic_vector (3 downto 0):= "0000";

begin

--Create the clock enable from fast clock:

--Slow clock:
process(clock)
begin
  if (reset = '1') then
            state_current <= await;
            motorSignal_I <= "0000";
    elsif (rising_edge(clock)) then -- opdaterer steps og state
        state_current <= state_next;
        if (state_current = await) then
            motorRunningOut <= '0';
        elsif (state_current = faseA) then
             motorSignal_I <= "1010";
             motorRunningOut <= '1';
        elsif (state_current = faseB) then
            motorSignal_I <= "0110";
            motorRunningOut <= '1';
        elsif (state_current = faseC) then
            motorSignal_I <= "0101";
            motorRunningOut <= '1';
        elsif (state_current = faseD) then
            motorSignal_I <= "1001";
            motorRunningOut <= '1';
        end if;
     end if;
end process;

--logik for stateskift
process(direction, runReqIn, state_current, motorSignal_I)
	begin
		case state_current is
		
			--await 
			when await =>
				--motorRunningOut <= '0';
				if (runReqIn = '0') then
					state_next <= await;
				elsif (runReqIn = '1') then
				    if	((motorSignal_I = "0000") or (direction = '1' and motorSignal_I = "1001")
									 or (direction = '0' and motorSignal_I = "0110")) then
					   state_next <= faseA;
				    elsif ((direction = '1' and motorSignal_I = "1010") or (direction = '0' and motorSignal_I = "0101")) then
					   state_next <= faseB;
				    elsif ((direction = '1' and motorSignal_I = "0110") or (direction = '0' and motorSignal_I = "1001")) then
					   state_next <= faseC;
				    elsif ((direction = '1' and motorSignal_I = "0101") or (direction = '0' and motorSignal_I = "1010")) then
					   state_next <= faseD;
					end if;
				end if;
				
			--faseA	
			when faseA =>
				--motorRunningOut <= '1';
				--motorSignal_I <= "1010";
				state_next <= await;
				
			--faseB
			when faseB =>
				--motorRunningOut <= '1';
				--motorSignal_I <= "0110";
				state_next <= await;
				
			--faseC
			when faseC =>
				--motorRunningOut <= '1';
				--motorSignal_I <= "0101";
                state_next <= await;
			
			--faseD
			when faseD =>
				--motorRunningOut <= '1';
				--motorSignal_I <= "1001";
				state_next <= await;
		end case;
	end process;
	motorSignal <= motorSignal_I;
end Behavioral;
