--Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
----------------------------------------------------------------------------------
--Tool Version: Vivado v.2020.2 (win64) Build 3064766 Wed Nov 18 09:12:45 MST 2020
--Date        : Sun May  9 02:20:11 2021
--Host        : DESKTOP-UGOUES3 running 64-bit major release  (build 9200)
--Command     : generate_target Processir_wrapper.bd
--Design      : Processir_wrapper
--Purpose     : IP block netlist
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity Processir_wrapper is
  port (
    I2C_scl_io : inout STD_LOGIC;
    I2C_sda_io : inout STD_LOGIC;
    PSOC_reset : out STD_LOGIC_VECTOR ( 0 to 0 );
    ack_motor : in STD_LOGIC;
    led : out STD_LOGIC_VECTOR ( 3 downto 0 );
    motor_in : in STD_LOGIC_VECTOR ( 8 downto 0 );
    motor_out : out STD_LOGIC_VECTOR ( 9 downto 0 );
    reset : in STD_LOGIC;
    rx_ESP : in STD_LOGIC;
    rx_PSOC : in STD_LOGIC;
    sys_clock : in STD_LOGIC;
    tx_ESP : out STD_LOGIC;
    tx_PSOC : out STD_LOGIC;
    usb_uart_rxd : in STD_LOGIC;
    usb_uart_txd : out STD_LOGIC;
    ventil_Pin : out STD_LOGIC_VECTOR ( 0 to 0 )
  );
end Processir_wrapper;

architecture STRUCTURE of Processir_wrapper is
  component Processir is
  port (
    reset : in STD_LOGIC;
    ventil_Pin : out STD_LOGIC_VECTOR ( 0 to 0 );
    rx_ESP : in STD_LOGIC;
    tx_ESP : out STD_LOGIC;
    rx_PSOC : in STD_LOGIC;
    tx_PSOC : out STD_LOGIC;
    led : out STD_LOGIC_VECTOR ( 3 downto 0 );
    sys_clock : in STD_LOGIC;
    motor_in : in STD_LOGIC_VECTOR ( 8 downto 0 );
    ack_motor : in STD_LOGIC;
    motor_out : out STD_LOGIC_VECTOR ( 9 downto 0 );
    PSOC_reset : out STD_LOGIC_VECTOR ( 0 to 0 );
    I2C_scl_i : in STD_LOGIC;
    I2C_scl_o : out STD_LOGIC;
    I2C_scl_t : out STD_LOGIC;
    I2C_sda_i : in STD_LOGIC;
    I2C_sda_o : out STD_LOGIC;
    I2C_sda_t : out STD_LOGIC;
    usb_uart_rxd : in STD_LOGIC;
    usb_uart_txd : out STD_LOGIC
  );
  end component Processir;
  component IOBUF is
  port (
    I : in STD_LOGIC;
    O : out STD_LOGIC;
    T : in STD_LOGIC;
    IO : inout STD_LOGIC
  );
  end component IOBUF;
  signal I2C_scl_i : STD_LOGIC;
  signal I2C_scl_o : STD_LOGIC;
  signal I2C_scl_t : STD_LOGIC;
  signal I2C_sda_i : STD_LOGIC;
  signal I2C_sda_o : STD_LOGIC;
  signal I2C_sda_t : STD_LOGIC;
begin
I2C_scl_iobuf: component IOBUF
     port map (
      I => I2C_scl_o,
      IO => I2C_scl_io,
      O => I2C_scl_i,
      T => I2C_scl_t
    );
I2C_sda_iobuf: component IOBUF
     port map (
      I => I2C_sda_o,
      IO => I2C_sda_io,
      O => I2C_sda_i,
      T => I2C_sda_t
    );
Processir_i: component Processir
     port map (
      I2C_scl_i => I2C_scl_i,
      I2C_scl_o => I2C_scl_o,
      I2C_scl_t => I2C_scl_t,
      I2C_sda_i => I2C_sda_i,
      I2C_sda_o => I2C_sda_o,
      I2C_sda_t => I2C_sda_t,
      PSOC_reset(0) => PSOC_reset(0),
      ack_motor => ack_motor,
      led(3 downto 0) => led(3 downto 0),
      motor_in(8 downto 0) => motor_in(8 downto 0),
      motor_out(9 downto 0) => motor_out(9 downto 0),
      reset => reset,
      rx_ESP => rx_ESP,
      rx_PSOC => rx_PSOC,
      sys_clock => sys_clock,
      tx_ESP => tx_ESP,
      tx_PSOC => tx_PSOC,
      usb_uart_rxd => usb_uart_rxd,
      usb_uart_txd => usb_uart_txd,
      ventil_Pin(0) => ventil_Pin(0)
    );
end STRUCTURE;
