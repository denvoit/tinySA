/*
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * The software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#pragma GCC push_options
#pragma GCC optimize ("Os")


static int VFO = 0;
static int points = 101; // For 's' and 'm' commands

VNA_SHELL_FUNCTION(cmd_mode)
{
  static const char cmd_low_high[] = "low|high";
  static const char cmd_in_out[] = "input|output";
  if (argc != 2) {
  usage:
    shell_printf("usage: mode %s %s\r\n", cmd_low_high,cmd_in_out);
    return;
  }
  int lh = get_str_index(argv[0], cmd_low_high);
  int io = get_str_index(argv[1], cmd_in_out);
  if (lh<0 || io<0)
    goto usage;
  menu_move_top();
  switch(lh+io*2)
  {
  case 0:
    set_mode(M_LOW);
    ui_mode_normal();
    break;
  case 1:
    set_mode(M_HIGH);
    ui_mode_normal();
    break;
  case 2:
    set_mode(M_GENLOW);
    menu_push_lowoutput();
    break;
  case 3:
    set_mode(M_GENHIGH);
    menu_push_highoutput();
    break;
  }
}

VNA_SHELL_FUNCTION(cmd_modulation )
{
  static const char cmd_mod[] = "off|AM_1kHz|AM_10Hz|NFM|WFM|extern";
  if (argc != 1) {
  usage:
    shell_printf("usage: modulation %s\r\n", cmd_mod);
    return;
  }
  static const int cmd_mod_val[] = { MO_NONE, MO_AM_1kHz, MO_AM_10Hz, MO_NFM, MO_WFM, MO_EXTERNAL};
  int m = get_str_index(argv[1], cmd_mod);
  if (m<0)
     goto usage;
  set_modulation(cmd_mod_val[m]);
}

VNA_SHELL_FUNCTION(cmd_spur)
{
  if (argc != 1) {
  usage:
    shell_printf("usage: spur on|off\r\n");
    return;
  }
  if (strcmp(argv[0],"on") == 0) {
    set_spur(1);
  } else if (strcmp(argv[0],"off") == 0) {
    set_spur(0);
  } else
    goto usage;
  redraw_request |= REDRAW_CAL_STATUS | REDRAW_AREA;
}

VNA_SHELL_FUNCTION(cmd_output)
{
  if (argc != 1) {
  usage:
    shell_printf("usage: output on|off\r\n");
    return;
  }
  if (strcmp(argv[0],"on") == 0) {
    setting.mute = false;
  } else if (strcmp(argv[0],"off") == 0) {
    setting.mute = true;
  } else
    goto usage;
  dirty = true;
}

VNA_SHELL_FUNCTION(cmd_load)
{
  if (argc != 1) {
  usage:
    shell_printf("usage: load 0..4\r\n");
    return;
  }
  int a = my_atoi(argv[0]);
  if (0 <= a && a <= 4) {
    caldata_recall(a);
  } else
    goto usage;
}


VNA_SHELL_FUNCTION(cmd_attenuate)
{
  if (argc != 1) {
//  usage:
    shell_printf("usage: attenuate 0..31|auto\r\n");
    return;
  }
  if (strcmp(argv[0],"auto") == 0) {
    if (!setting.auto_attenuation)
      set_auto_attenuation();
  } else {
    int a = my_atoi(argv[0]);
//    if (a < 0 || a>31)
//      goto usage;
    if (get_attenuation() != a)
      set_attenuation(a);
  }
  redraw_request |= REDRAW_CAL_STATUS | REDRAW_AREA;
}

VNA_SHELL_FUNCTION(cmd_level)
{
  if (argc != 1) {
    if (setting.mode==M_GENLOW)
      shell_printf("usage: level -76..-6\r\n");
    if (setting.mode==M_GENHIGH)
      shell_printf("usage: level -38..13\r\n");
    return;
  }
  float f = my_atof(argv[0]);
  set_level(f);
}

VNA_SHELL_FUNCTION(cmd_sweeptime)
{
  if (argc != 1) {
    shell_printf("usage: sweeptime 0.003..60\r\n");
    return;
  }
  float f = my_atof(argv[0]);
  set_sweep_time_us(f*ONE_SECOND_TIME);
}




VNA_SHELL_FUNCTION(cmd_levelchange)
{
  if (argc != 1) {
    shell_printf("usage: levelchange -70..+70\r\n");
    return;
  }
  float f = my_atof(argv[0]);
  set_level_sweep(f);
}

VNA_SHELL_FUNCTION(cmd_leveloffset)
{
  if (argc == 0) {
    shell_printf("leveloffset low %.1f\r\n", config.low_level_offset);
    shell_printf("leveloffset high %.1f\r\n", config.high_level_offset);
    return;
  } else if (argc == 2) {
    float v = my_atof(argv[1]);
    if (strcmp(argv[0],"low") == 0)
      config.low_level_offset = v;
    else if (strcmp(argv[0],"high") == 0)
      config.high_level_offset = v;
    else
      goto usage;
  } else {
  usage:
    shell_printf("leveloffset [low|high] [<offset>]\r\n");
  }
}

VNA_SHELL_FUNCTION(cmd_deviceid)
{
  if (argc == 0) {
    shell_printf("deviceid %d\r\n", config.deviceid);
    return;
  } else if (argc == 1) {
    float v = my_atoui(argv[0]);
    config.deviceid = v;
  } else {
    shell_printf("usage: deviceid [<number>]\r\n");
  }
}


VNA_SHELL_FUNCTION(cmd_rbw)
{
  if (argc != 1) {
  usage:
    shell_printf("usage: rbw 2..600|auto\r\n");
    return;
  }
  if (get_str_index(argv[0], "auto|0")>=0) {
    if (setting.rbw_x10 != 0)
      set_RBW(0);
  } else {
    uint32_t a = my_atoui(argv[0]);
    if (a < 2 || a>600)
      goto usage;
    if (setting.rbw_x10 != a*10)
      set_RBW(a*10);
 }
}

VNA_SHELL_FUNCTION(cmd_if)
{
  if (argc != 1) {
  usage:
    shell_printf("usage: if {433M..435M}\r\n");
    return;
  } else {
    int a = my_atoi(argv[0]);
    if (a!= 0 &&( a < 433000000 || a>435000000))
      goto usage;
    setting.auto_IF = false;
    set_IF(a);
  }
}


VNA_SHELL_FUNCTION(cmd_trigger)
{
  if (argc == 0)
    goto usage;

  if (( '0' <= argv[0][0] && argv[0][0] <= '9') || argv[0][0] == '-') {
    float t = my_atof(argv[0]);
    if (setting.trigger == T_AUTO )
      set_trigger(T_NORMAL);
    set_trigger_level(to_dBm(t));
    goto update;
  }
  static const char cmd_trigger_list[] = "auto|normal|single";
  if (argc == 1) {
    int type = get_str_index(argv[0], cmd_trigger_list);
    if (type >= 0) {
      set_trigger(type);
      goto update;
    }
    goto usage;
  }
update:
  redraw_request |= REDRAW_CAL_STATUS | REDRAW_AREA;
  completed = true;
  return;
usage:
  shell_printf("trigger {value}\r\n"\
               "trigger {%s}\r\n" , cmd_trigger_list);
}


VNA_SHELL_FUNCTION(cmd_v)
{
    if (argc != 1) {
        shell_printf("%d\r\n", VFO);
        return;
    }
    VFO = my_atoi(argv[0]) > 0 ? 1 : 0;
    shell_printf("VFO %d\r\n", VFO);
}

VNA_SHELL_FUNCTION(cmd_y)
{
  int rvalue;
  int lvalue = 0;
  if (argc != 1 && argc != 2) {
    shell_printf("usage: y {addr(0-95)} [value(0-0xFF)]\r\n");
    return;
  }
  rvalue = my_atoui(argv[0]);
#ifdef __SI4432__
  SI4432_Sel = VFO;
  if (argc == 2){
    lvalue = my_atoui(argv[1]);
    SI4432_Write_Byte(rvalue, lvalue);
  } else {
    lvalue = SI4432_Read_Byte(rvalue);
    shell_printf("%x\r\n", lvalue);
  }
#endif
}

VNA_SHELL_FUNCTION(cmd_selftest)
{
  if (argc < 1 || argc > 2) {
    shell_printf("usage: selftest (1-3) [arg]\r\n");
    return;
  }
  setting.test = my_atoi(argv[0]);
  if (argc == 1)
    setting.test_argument = 0;
  else
    setting.test_argument = my_atoi(argv[1]);
  sweep_mode = SWEEP_SELFTEST;
}

#ifdef __ULTRA_SA__
VNA_SHELL_FUNCTION(cmd_x)
{
  uint32_t reg;


  if (argc != 1) {
    shell_printf("usage: x value(0-FFFFFFFF)\r\n");
    return;
  }
  reg = xtoi(argv[0]);

  if ((reg & 7) == 5) {
   if (reg & (1<<22))
      VFO = 1;
    else
      VFO = 0;
   reg &= ~0xc00000;    // Force led to show lock
   reg |=  0x400000;
  }

  ADF4351_WriteRegister32(VFO, reg);
  shell_printf("x=%x\r\n", reg);
}
#endif


VNA_SHELL_FUNCTION(cmd_i)
{
  int rvalue;
return;             // Don't use!!!!
#ifdef __SI4432__
  SI4432_Init();
  shell_printf("SI4432 init done\r\n");
  if (argc == 1) {
    rvalue = my_atoui(argv[0]);
    set_switches(rvalue);
    set_mode(rvalue);
    shell_printf("SI4432 mode %d set\r\n", rvalue);
  }
#endif
}

VNA_SHELL_FUNCTION(cmd_o)
{
  (void) argc;
  uint32_t value = my_atoi(argv[0]);
  if (VFO == 0)
    setting.frequency_IF = value;
  set_freq(VFO, value);
}

VNA_SHELL_FUNCTION(cmd_d)
{
  (void) argc;
  (void) argv;
  int32_t a = my_atoi(argv[0]);
  setting.drive=a;
  dirty = true;
}

#if 0
extern int16_t adc_buf_read(uint16_t *result, uint32_t count);

VNA_SHELL_FUNCTION(cmd_g)
{
  (void) argc;
  (void) argv;
  int32_t a = my_atoi(argv[0]);
  systime_t start_of_read = chVTGetSystemTimeX();
  adc_buf_read(spi_buffer, 256);
  systime_t time_of_read = chVTGetSystemTimeX() - start_of_read;
  shell_printf("Time: %d\r\n", time_of_read);
  for (int i=0;i<20;i++)
    shell_printf("[%d] = %d\r\n", (int)i, (int)(spi_buffer[i]));
}
#endif


VNA_SHELL_FUNCTION(cmd_a)
{
  (void)argc;
  if (argc != 1) {
    shell_printf("a=%u\r\n", frequencyStart);
    return;
  }
  uint32_t value = my_atoui(argv[0]);
  frequencyStart = value;
}


VNA_SHELL_FUNCTION(cmd_b)
{
  (void)argc;
  if (argc != 1) {
    shell_printf("b=%u\r\n", frequencyStop);
    return;
  }
  uint32_t value = my_atoui(argv[0]);
  frequencyStop = value;
}

VNA_SHELL_FUNCTION(cmd_t)
{
  (void)argc;
  (void)argv;
}

VNA_SHELL_FUNCTION(cmd_e)
{
  (void)argc;
  if (argc != 1) {
    shell_printf("e=%d\r\n", setting.tracking);
    return;
  }
  setting.tracking = my_atoi(argv[0]);
  if (setting.tracking == -1)
    setting.tracking = false;
  else
    setting.tracking = true;

  if (argc >1)
    frequencyExtra = my_atoi(argv[1]);
}

VNA_SHELL_FUNCTION(cmd_s)
{
  (void)argc;
  if (argc != 1) {
    shell_printf("s=%d\r\n", points);
    return;
  }
  points = my_atoi(argv[0]);
}

void sweep_remote(void)
{
  uint32_t i;
  uint32_t step = (points - 1);
  uint32_t span = frequencyStop - frequencyStart;
  uint32_t delta = span / step;
  uint32_t error = span % step;
  uint32_t f = frequencyStart - setting.frequency_IF, df = step>>1;
  uint32_t old_step = setting.frequency_step;
  setting.frequency_step = delta;
  streamPut(shell_stream, '{');
  dirty = true;
  for (i = 0; i <= step; i++, f+=delta) {
    if (operation_requested)
      break;
    int val = perform(false, i, f, false) + float_TO_PURE_RSSI(EXT_ZERO_LEVEL);
    streamPut(shell_stream, 'x');
    streamPut(shell_stream, (uint8_t)(val & 0xFF));
    streamPut(shell_stream, (uint8_t)((val>>8) & 0xFF));
    df+=error;if (df >=step) {f++;df -= step;}
  }
  streamPut(shell_stream, '}');
  setting.frequency_step = old_step;
  sweep_mode = 0;
}

VNA_SHELL_FUNCTION(cmd_m)
{
  (void)argc;
  (void)argv;

//  set_mode(0);
//  setting.tracking = false; //Default test setup
//  setting.step_atten = false;
//  set_attenuation(0);
//  set_reflevel(-10);
//  set_sweep_frequency(ST_START,frequencyStart - setting.frequency_IF );
//  set_sweep_frequency(ST_STOP, frequencyStop - setting.frequency_IF);
//  draw_cal_status();

  pause_sweep();
//  update_rbw();
  chThdSleepMilliseconds(10);
  sweep_mode = SWEEP_REMOTE;
//  update_rbw();
}

VNA_SHELL_FUNCTION(cmd_p)
{
  (void)argc;
return;
  int p = my_atoi(argv[0]);
  int a = my_atoi(argv[1]);
  if (p==5)
    set_attenuation(-a);
  if (p==6)
    set_mode(a);
  if (p==1)
    if (get_refer_output() != a)
      set_refer_output(a);
}

VNA_SHELL_FUNCTION(cmd_w)
{
  (void)argc;
  int p = my_atoi(argv[0]);
return;
  set_RBW(p*10);
}

VNA_SHELL_FUNCTION(cmd_f)
{
  (void)argc;
  setting.test = 5;
  setting.test_argument =  my_atoi(argv[0]);;
  sweep_mode = SWEEP_SELFTEST;
}

VNA_SHELL_FUNCTION(cmd_correction)
{
  (void)argc;
  if (argc == 0) {
    shell_printf("index frequency value\r\n");
    for (int i=0; i<CORRECTION_POINTS; i++) {
      shell_printf("%d %d %.1f\r\n", i, config.correction_frequency[i], config.correction_value[i]);
    }
    return;
  }
  if (argc == 1 && (strcmp(argv[0],"reset") == 0)) {
    for (int i=0; i<CORRECTION_POINTS; i++) {
      config.correction_value[i] = 0.0;
    }
    shell_printf("correction table reset\r\n");
    return;
  }
  if (argc != 3) {
    shell_printf("usage: correction 0-9 frequency(Hz) value(dB)\r\n");
    return;
  }
  int i = my_atoi(argv[0]);
  uint32_t f = my_atoui(argv[1]);
  float v = my_atof(argv[2]);
  config.correction_frequency[i] = f;
  config.correction_value[i] = v;
  redraw_request|=REDRAW_AREA;                  // to ensure the change in level will be visible
  shell_printf("updated %d to %d %.1f\r\n", i, config.correction_frequency[i], config.correction_value[i]);
}

VNA_SHELL_FUNCTION(cmd_scanraw)
{
  uint32_t start, stop;
  uint32_t points = sweep_points;
  if (argc < 2 || argc > 3) {
    shell_printf("usage: scanraw {start(Hz)} {stop(Hz)} [points]\r\n");
    return;
  }

  start = my_atoui(argv[0]);
  stop = my_atoui(argv[1]);
  if (start > stop) {
      shell_printf("frequency range is invalid\r\n");
      return;
  }
  if (argc == 3) {
    points = my_atoi(argv[2]);
  }
  uint32_t old_step = setting.frequency_step;
  float f_step = (stop-start)/ points;
  setting.frequency_step = (uint32_t)f_step;

  streamPut(shell_stream, '{');
  static  uint32_t old_start=0, old_stop=0, old_points=0;
  if (old_start != start || old_stop != stop || old_points != points) {
    dirty = true;
    old_start = start;
    old_stop = stop;
    old_points = points;
  }
  operation_requested = false;
  dirty = true;

  for (uint32_t i = 0; i<points; i++) {
    int val = perform(false, i, start +(uint32_t)(f_step * i), false) + float_TO_PURE_RSSI(EXT_ZERO_LEVEL);
    if (operation_requested) // break on operation in perform
      break;
    streamPut(shell_stream, 'x');
    streamPut(shell_stream, (uint8_t)(val & 0xFF));
    streamPut(shell_stream, (uint8_t)((val>>8) & 0xFF));
  }
  streamPut(shell_stream, '}');
  setting.frequency_step = old_step;
  redraw_request = 0; // disable screen update in this mode
}

VNA_SHELL_FUNCTION(cmd_caloutput)
{
  static const char cmd[] = "off|30|15|10|4|3|2|1";
  if (argc != 1) {
  usage:
    shell_printf("usage: caloutput %s\r\n", cmd);
    return;
  }
  int m = get_str_index(argv[0], cmd);
  if (m != -1)
    set_refer_output(m - 1);
}


#pragma GCC pop_options


