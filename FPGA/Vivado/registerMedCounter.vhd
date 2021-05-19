----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 04.05.2021 13:18:16
-- Design Name: 
-- Module Name: registerMedCounter - Behavioral
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


entity registerMedCounter is

    generic (
        g_init_val : std_logic := '0'
    );

    Port ( dataIn : in unsigned(7 downto 0) := (others => g_init_val) ;
           newData : in STD_LOGIC := g_init_val;
           motorRunning : in STD_LOGIC := g_init_val;
           clock : in STD_LOGIC;
           reset : in STD_LOGIC := g_init_val;
           dataOut : out unsigned(7 downto 0) := (others => g_init_val) ;
           runRequest : out STD_LOGIC := g_init_val;
           ack : out STD_LOGIC := g_init_val);
end registerMedCounter;

architecture Behavioral of registerMedCounter is
    type registerState is (inactive, recieveData, updateSteps, motorKoerer, betweenSteps);
	signal current_state, next_state: registerState;
	signal steps: unsigned(7 downto 0):= "00000000";
    
begin
	--event for at hoppe til naeste state
	process(clock, reset) is 
	begin
		if (reset = '1') then
			current_state <= inactive;
			steps <= (others => '0');
		elsif (rising_edge(clock)) then -- opdaterer steps og state
		    if ((current_state = updateSteps) and (steps /= "0")) then
		    --if (current_state = updateSteps) then
		      steps <= steps - "1";
		    elsif (current_state = recieveData) then
		      steps <= dataIn;
		    end if;
            current_state <= next_state;
		end if;
	end process;
	
	--logik for stateskift
	process(newData, motorRunning, dataIn, current_state)
	begin
	   case current_state is
	       --inactive
	       when inactive =>
	           if (newData = '0') then
					next_state <= inactive;
				elsif (newData = '1') then
					next_state <= recieveData;
				end if;
				
	       --recieveData
			when recieveData =>
			     if (newData = '1') then
					ack <= '1';
					next_state <= recieveData;
				elsif (newData = '0') then
					next_state <= motorKoerer;
				end if;
			     			
			--motorKoerer
			when motorKoerer =>
				ack <= '0';
				if (steps = "00000000") then
					next_state <= inactive;
				elsif (motorRunning = '0') then
					next_state <= motorKoerer;
					runRequest <= '1';
				elsif (motorRunning = '1') then
					next_state <= updateSteps;
				end if;
			
			--updateSteps
			when updateSteps =>
				runRequest <= '0';
				if (newData = '1') then
					next_state <= recieveData;
				else 
					next_state <= betweenSteps;
				end if;
			
			--betweenSteps
			when betweenSteps =>
			     if (motorRunning = '1') then
			         next_state <= betweenSteps;
			     elsif (motorRunning = '0') then
			         next_state <= motorKoerer;
			     end if;
			
		end case;
	end process;
	
	dataOut <= steps;

end Behavioral;
