----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 04.05.2021 13:18:16
-- Design Name: 
-- Module Name: motorRegWrapper - Behavioral
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
library work;
use work.all;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity motorRegWrapper is

    generic (
        g_init_val : std_logic := '0'
    );

    Port ( clk : in STD_LOGIC;
           reset : in STD_LOGIC := g_init_val;
           dataIn : in STD_LOGIC_VECTOR (7 downto 0) := (others => g_init_val) ;
           dataOut : out STD_LOGIC_VECTOR (7 downto 0) := (others => g_init_val) ;
           ack : out STD_LOGIC := g_init_val;
           newData : in STD_LOGIC := g_init_val;
           motorSignal : out STD_LOGIC_VECTOR (3 downto 0) := (others => g_init_val) ;
           direction : in STD_LOGIC := g_init_val);
           
end motorRegWrapper;

architecture Behavioral of motorRegWrapper is

signal i_running, i_run, slow_i_clock: std_logic;

begin
    motorStyring: entity work.motorStyring 
    port map(
        clock => slow_i_clock,
        reset => reset,
        runReqIn => i_run,
        direction => direction,
        motorRunningOut => i_running,
        motorSignal => motorSignal);

    registerStyring: entity work.registerMedCounter
    port map(
        clock => clk,
        reset => reset,
        newData => newData,
        motorRunning => i_running,
        dataIn => unsigned(dataIn),
        std_logic_vector(dataOut) => dataOut,
        runRequest => i_run,
        ack => ack);

    clockSlower: entity work.Clock_Divider
    port map(
        clk => clk,
        reset => reset,
        Clock_out => slow_i_clock);
        
   

end Behavioral;
