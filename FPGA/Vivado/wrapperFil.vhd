----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 03.05.2021 15:23:26
-- Design Name: 
-- Module Name: wrapperFil - Behavioral
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
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;

entity wrapperFil is
  port (
    --clk:     in std_logic;
    reset:     in std_logic; 
    --dir:         in std_logic;
    --newData: in std_logic;
    --dataIn:     in std_logic_vector (7 downto 0);
    --dataOut: out std_logic_vector (7 downto 0);
    motorUd:    out std_logic_vector (3 downto 0);
    --ack:         out std_logic;
    
    I2C_scl_io : inout STD_LOGIC;
    I2C_sda_io : inout STD_LOGIC;
    PSOC_reset : out STD_LOGIC_VECTOR ( 0 to 0 );
    --ack_motor : in STD_LOGIC;
    led : out STD_LOGIC_VECTOR ( 3 downto 0 );
    --motor_in : in STD_LOGIC_VECTOR ( 8 downto 0 );
    --motor_out : out STD_LOGIC_VECTOR ( 9 downto 0 );
    --reset : in STD_LOGIC;
    rx_ESP : in STD_LOGIC;
    rx_PSOC : in STD_LOGIC;
    sys_clock : in STD_LOGIC;
    tx_ESP : out STD_LOGIC;
    tx_PSOC : out STD_LOGIC;
    usb_uart_rxd : in STD_LOGIC;
    usb_uart_txd : out STD_LOGIC;
    ventil_Pin : out STD_LOGIC_VECTOR ( 0 to 0 )
  );
end wrapperFil;


architecture Behavioral of wrapperFil is
signal motorAck, direction, newData : std_logic;
signal dataIndReg : std_logic_vector ( 7 downto 0 );
signal dataUdReg : std_logic_vector ( 7 downto 0 );


begin
    processor: entity work.Processir_wrapper 
        port map(
            I2C_scl_io => I2C_scl_io,
            I2C_sda_io => I2C_sda_io,
            PSOC_reset => PSOC_reset,
            ack_motor => motorAck,
            led => led,
            motor_in(7 downto 0) => dataUdReg,
            motor_in(8) => direction,
            motor_out(7 downto 0) => dataIndReg,
            motor_out(8) => direction,
            motor_out(9) => newData,
            reset => reset,
            rx_ESP => rx_ESP,
            rx_PSOC => rx_PSOC,
            sys_clock => sys_clock,
            tx_ESP => tx_ESP,
            tx_PSOC => tx_PSOC,
            usb_uart_rxd => usb_uart_rxd,
            usb_uart_txd => usb_uart_txd,
            ventil_Pin => ventil_Pin);
    
    motorKomponent: entity work.motorRegWrapper
        port map(
            clk => sys_clock,
            ack => motorAck,
            reset => reset,
            direction => direction,
            newData => newData,
            dataIn => dataIndReg,
            dataOut => dataUdReg,
            motorSignal => motorUd);

end Behavioral;
