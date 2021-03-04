/*
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

uint32_t xtoi(char *t);

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
  static const char cmd_mod[] = "off|am|nfm|wfm|extern|freq";
  if (argc < 1) {
  usage:
    shell_printf("usage: modulation %s 100..6000\r\n", cmd_mod);
    return;
  }
  static const int cmd_mod_val[] = { MO_NONE, MO_AM, MO_NFM, MO_WFM, MO_EXTERNAL, -1};
  int m = get_str_index(argv[0], cmd_mod);
  if (m<0)
     goto usage;
  if (cmd_mod_val[m] >=0)
    set_modulation(cmd_mod_val[m]);
  else {
    if (argc != 2)
      goto usage;
    int a = my_atoi(argv[1]);
    set_modulation_frequency(a);
  }
}

int generic_option_cmd( const char *cmd, const char *cmd_list, int argc, char *argv)
{
  if (argc != 1) {
  usage:
    shell_printf("usage: %s %s\r\n", cmd, cmd_list);
    return -1;
  }
  int m = get_str_index(argv, cmd_list);
  if (m < 0)
    goto usage;
  return m;
}


VNA_SHELL_FUNCTION(cmd_spur)
{
//  static const char cmd[] = "off|on";
//  if (argc != 1) {
//  usage:
//    shell_printf("usage: spur %s\r\n", cmd);
//    return;
//  }
#ifdef TINYSA4
  int m = generic_option_cmd("spur", "off|on|auto", argc, argv[0]);
#else
  int m = generic_option_cmd("spur", "off|on", argc, argv[0]);
#endif
  if (m>=0) {
    set_spur(m);
    redraw_request |= REDRAW_CAL_STATUS | REDRAW_AREA;
  }
}

#ifdef TINYSA4
VNA_SHELL_FUNCTION(cmd_lna)
{
//  static const char cmd[] = "off|on";
//  if (argc != 1) {
//  usage:
//    shell_printf("usage: spur %s\r\n", cmd);
//    return;
//  }
  int m = generic_option_cmd("lna", "off|on", argc, argv[0]);
  if (m>=0) {
    set_extra_lna(m);
    redraw_request |= REDRAW_CAL_STATUS | REDRAW_AREA;
  }
}

VNA_SHELL_FUNCTION(cmd_ultra)
{
//  static const char cmd[] = "off|on";
//  if (argc != 1) {
//  usage:
//    shell_printf("usage: spur %s\r\n", cmd);
//    return;
//  }
  int m = generic_option_cmd("ultra", "off|on", argc, argv[0]);
  if (m>=0) {
    config.ultra = m;
    update_min_max_freq();
  }
}
#endif

VNA_SHELL_FUNCTION(cmd_output)
{
#if 0
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
#endif
  int m = generic_option_cmd("output", "on|off", argc, argv[0]);
  if (m>=0) {
    setting.mute = m;
    dirty = true;
  }
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



VNA_SHELL_FUNCTION(cmd_offset)
{
  if (argc != 1) {
    shell_printf("usage: offset -100.0..+100.0\r\n");
    return;
  }
  float o = my_atof(argv[0]);
  set_offset(o);
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
    const char *p = "leveloffset %s %.1f\r\n";
    shell_printf(p, "low",          config.low_level_offset);
    shell_printf(p, "high",         config.high_level_offset);
    shell_printf(p, "low output",   config.low_level_output_offset);
    shell_printf(p, "high output",  config.high_level_output_offset);
    shell_printf(p, "switch",       config.switch_offset);
    return;
  } else if (argc == 2) {
    float v = my_atof(argv[1]);
    if (strcmp(argv[0],"low") == 0)
      config.low_level_offset = v;
    else if (strcmp(argv[0],"high") == 0)
      config.high_level_offset = v;
    else if (strcmp(argv[0],"switch") == 0)
      config.switch_offset = v;
    else
      goto usage;
    dirty = true;
  } else if (argc == 3 && strcmp(argv[1],"output") == 0) {
    float v = my_atof(argv[2]);
    if (strcmp(argv[0],"low") == 0)
      config.low_level_output_offset = v;
    else if (strcmp(argv[0],"high") == 0)
      config.high_level_output_offset = v;
    else
      goto usage;
    dirty = true;
  } else {
  usage:
    shell_printf("leveloffset [low|high|switch] {output} [-20..+20]\r\n");
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

VNA_SHELL_FUNCTION(cmd_sweep_voltage)
{
  float value;
  if (argc != 1) {
    shell_printf("usage: sweep_voltage {value(0-3.3)}\r\n"\
                 "current value: %f\r\n", config.sweep_voltage);
    return;
  }
  value = my_atof(argv[0]);
  config.sweep_voltage = value;
}

VNA_SHELL_FUNCTION(cmd_rbw)
{
  if (argc != 1) {
  usage:
#ifdef TINYSA4
	shell_printf("usage: rbw 0.3..600|auto\r\n");
#else
	shell_printf("usage: rbw 2..600|auto\r\n");
#endif
	return;
  }
  if (get_str_index(argv[0], "auto|0")>=0) {
    if (setting.rbw_x10 != 0)
      set_RBW(0);
  } else {
    float a = my_atof(argv[0]);
    if (a < 0.2 || a>600)
      goto usage;
    if (setting.rbw_x10 != a*10)
      set_RBW((int) ( a*10));
 }
}

VNA_SHELL_FUNCTION(cmd_if)
{
  if (argc != 1) {
  usage:
    shell_printf("usage: if {433M..435M}\r\n%QHz\r\n", setting.frequency_IF);
    return;
  } else {
    freq_t a = (freq_t)my_atoi(argv[0]);
    if (a!= 0 &&( a < (DEFAULT_IF - (freq_t)2000000) || a>(DEFAULT_IF + (freq_t)2000000)))
      goto usage;
    setting.auto_IF = false;
    set_IF(a);
  }
}

#ifdef TINYSA4
VNA_SHELL_FUNCTION(cmd_ultra_start)
{
  if (argc != 1) {
    shell_printf("usage: ultra_start {0..4290M}\r\n%qHz\r\n", config.ultra_threshold);
    return;
  } else {
    freq_t a = (freq_t)my_atoi(argv[0]);
    config.ultra_threshold = a;
    config_save();
  }
}


VNA_SHELL_FUNCTION(cmd_if1)
{
  if (argc != 1) {
  usage:
    shell_printf("usage: if1 {975M..979M}\r\n%qHz\r\n", config.frequency_IF1);
    return;
  } else {
    freq_t a = (freq_t)my_atoi(argv[0]);
    if (a!= 0 &&( a < (DEFAULT_IF - (freq_t)80000000) || a>(DEFAULT_IF + (freq_t)80000000)))
      goto usage;
    config.frequency_IF1 = a;
    config_save();
  }
}
#endif

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

#ifdef __SINGLE_LETTER__

static int VFO = 0;


uint32_t xtoi(char *t)
{

  uint32_t v=0;
  while (*t) {
    if ('0' <= *t && *t <= '9')
      v = v*16 + *t - '0';
    else if ('a' <= *t && *t <= 'f')
      v = v*16 + *t - 'a' + 10;
    else if ('A' <= *t && *t <= 'F')
      v = v*16 + *t - 'A' + 10;
    else
      return v;
    t++;
  }
  return v;
}

#ifdef __ADF4351__

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
  (void)argc;
  (void)argv;
return;             // Don't use!!!!
#ifdef __SI4432__
int rvalue;
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
  freq_t value = my_atoi(argv[0]);
  if (VFO == 0)
    setting.frequency_IF = value;
  set_freq(VFO, value);
}

VNA_SHELL_FUNCTION(cmd_d)
{
  (void) argc;
  (void) argv;
  int32_t a = my_atoi(argv[0]);
#ifdef TINYSA4
  int32_t d;
  if (argc == 2)
    d = my_atoi(argv[1]);
  else {
    d = a;
    a = 2;
  }
  switch (a) {
  case 1:
    SI4463_set_output_level(d);
    break;
  case 2:
    ADF4351_drive (d);
    break;
  case 3:
    ADF4351_aux_drive(d);
    break;
  }
//  setting.lo_drive=a;
//  dirty = true;
#else
  setting.lo_drive=a;
  dirty = true;
#endif
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
    shell_printf("a=%U\r\n", frequencyStart);
    return;
  }
  freq_t value = my_atoui(argv[0]);
  frequencyStart = value;
}


VNA_SHELL_FUNCTION(cmd_b)
{
  (void)argc;
  if (argc != 1) {
    shell_printf("b=%U\r\n", frequencyStop);
    return;
  }
  freq_t value = my_atoui(argv[0]);
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
  if (argc < 1) {
    shell_printf("usage: y {addr(0-95)} [value(0-0xFF)]\r\n");
    return;
  }
#ifdef __SI4432__
  int lvalue = 0;
  int rvalue;
  rvalue = xtoi(argv[0]);
  SI4432_Sel = VFO;
  if (argc == 2){
    lvalue = my_atoui(argv[1]);
    SI4432_Write_Byte(rvalue, lvalue);
  } else {
    lvalue = SI4432_Read_Byte(rvalue);
    shell_printf("%x\r\n", lvalue);
  }
#endif
#ifdef __SI4463__
  uint8_t data[16];
  data[0] = xtoi(argv[0]);
  for (int i=1; i < argc; i++) {
    data[i] = xtoi(argv[i]);
  }
  SI4463_do_api(data, argc, data, 16);
  for (int i=0; i<16; i++)
    shell_printf("%02x ", data[i]);
  shell_printf("\r\n");
#endif
}
#if 0       // not used
VNA_SHELL_FUNCTION(cmd_z)
{
  static const char cmd_z_list[] = "t|r|i";
  if (argc != 1) {
    shell_printf("usage: z %s\r\n", cmd_z_list);
    return;
  }
  if (argc == 1) {
#ifdef __SI4432__
    SI4432_Sel = VFO;
    int type = get_str_index(argv[0], cmd_z_list);
    switch(type) {
    case 0:
      SI4432_Transmit(3);
      break;
    case 1:
      SI4432_Receive();
      break;
    case 2:
      SI4432_Reset();
      break;
    }
#endif
  }
}
#endif


#endif

#ifdef __SINGLE_LETTER__

void sweep_remote(void)
{
  uint32_t i;
  uint32_t step = (points - 1);
  freq_t span = frequencyStop - frequencyStart;
  freq_t delta = span / step;
  freq_t error = span % step;
  freq_t f = frequencyStart - setting.frequency_IF, df = step>>1;
  freq_t old_step = setting.frequency_step;
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
  int p = my_atoi(argv[0]);
#ifdef TINYSA4
  SI4463_set_output_level(p);
#endif
return;
  int a = my_atoi(argv[1]);
  if (p==5)
    set_attenuation(-a);
  if (p==6)
    set_mode(a);
  if (p==1)
    if (get_refer_output() != a)
      set_refer_output(a);
}

#ifdef TINYSA4
VNA_SHELL_FUNCTION(cmd_g)
{
  (void)argc;
  int p = my_atoi(argv[0]);
  int a = my_atoi(argv[1]);
  SI4463_set_gpio(p,a);
}
#endif

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
#endif

VNA_SHELL_FUNCTION(cmd_correction)
{
  (void)argc;
  if (argc == 0) {
    shell_printf("index frequency value\r\n");
    for (int i=0; i<CORRECTION_POINTS; i++) {
      shell_printf("%d %D%.1f\r\n", i, setting.correction_frequency[i], setting.correction_value[i]);
    }
    return;
  }
  if (argc == 1 && (strcmp(argv[0],"reset") == 0)) {
    for (int i=0; i<CORRECTION_POINTS; i++) {
      setting.correction_value[i] = 0.0;
    }
    shell_printf("correction table reset\r\n");
    return;
  }
  if (argc != 3) {
    shell_printf("usage: correction 0-9 frequency(Hz) value(dB)\r\n");
    return;
  }
  int i = my_atoi(argv[0]);
  freq_t f = my_atoui(argv[1]);
  float v = my_atof(argv[2]);
  setting.correction_frequency[i] = f;
  setting.correction_value[i] = v;
  redraw_request|=REDRAW_AREA;                  // to ensure the change in level will be visible
  shell_printf("updated %d to %D %.1f\r\n", i, setting.correction_frequency[i], setting.correction_value[i]);
}

VNA_SHELL_FUNCTION(cmd_scanraw)
{
  freq_t start, stop;
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

//  if (get_waterfall())
//    disable_waterfall();            // display dma hangs when waterfall is enabled

  freq_t old_step = setting.frequency_step;
  float f_step = (stop-start)/ points;
  setting.frequency_step = (freq_t)f_step;

  streamPut(shell_stream, '{');
  static freq_t old_start=0, old_stop=0;
  static uint32_t old_points=0;
  if (old_start != start || old_stop != stop || old_points != points) {     // To prevent dirty for every sweep
    dirty = true;
    old_start = start;
    old_stop = stop;
    old_points = points;
  }
  operation_requested = false;
  dirty = true;

  for (uint32_t i = 0; i<points; i++) {
    int val = perform(false, i, start +(freq_t)(f_step * i), false) + float_TO_PURE_RSSI(EXT_ZERO_LEVEL);
    if (operation_requested && SDU1.config->usbp->state != USB_ACTIVE) // break on operation in perform
      break;
    streamPut(shell_stream, 'x');
    streamPut(shell_stream, (uint8_t)(val & 0xFF));
    streamPut(shell_stream, (uint8_t)((val>>8) & 0xFF));
    if ((i & 0x07) == 0) {  // if required
      int pos = i * (WIDTH+1) / points;
      ili9341_set_background(LCD_SWEEP_LINE_COLOR);
      ili9341_fill(OFFSETX, CHART_BOTTOM+1, pos, 1);     // update sweep progress bar
      ili9341_set_background(LCD_BG_COLOR);
      ili9341_fill(OFFSETX+pos, CHART_BOTTOM+1, WIDTH-pos, 1);
    }

  }
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_fill(OFFSETX, CHART_BOTTOM+1, WIDTH, 1);
  streamPut(shell_stream, '}');
  setting.frequency_step = old_step;
  dirty = true;
  redraw_request = 0; // disable screen update in this mode
}

VNA_SHELL_FUNCTION(cmd_caloutput)
{
  static const char cmd[] = "off|30|15|10|4|3|2|1";
  if (argc != 1) {
    shell_printf("usage: caloutput %s\r\n", cmd);
    return;
  }
  int m = get_str_index(argv[0], cmd);
  if (m != -1)
    set_refer_output(m - 1);
}


#pragma GCC pop_options


