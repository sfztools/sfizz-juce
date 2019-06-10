# Sfizz

Sfizz is JUCE based sample player that aims at supporting a healthy part of the `.sfz` virtual instrument format.
It's still very experimental so expect many bugs and weird stuff.
Any help in testing is greatly appreciated.
If you can code, that's perfect.
The easiest is to load the project through JUCE's Projucer [(see here)](https://juce.com/discover/stories/projucer-manual) and export something that matches your preferred IDE.
There is a CMakeFile but it's quite custom and not very flexible yet.
If you can't or don't want to code, that's fine too, just load your favorite virtual instrument and tell me what seems wrong :)

# Development status

These are a list of opcodes support.
The currently supported ones are marked :heavy_check_mark:, the registered but unsupported ones are :x:.
Opcodes that do not register and set a corresponding variable in the region objects are not listed, so any missing opcode here is basically completely unsupported.
I try to unit-test some parts of the system but sometimes it's quite messy to do at first, so only "manual" empirical tests are done.
The classification follows the list over at https://sfzformat.com/.

## Sound source
|       opcode        | type  | default value |   range   |       status       |   tested    |
| ------------------- | ----- | ------------- | --------- | ------------------ | ----------- |
| sample              | /     | --            |           | :heavy_check_mark: | Unit test   |
| sample (generators) | /     | --            |           | :heavy_check_mark: | Manual test |
| delay               | float | 0             | 0 - 100   | :heavy_check_mark: | Manual test |
| delay_random        | float | 0             | 0 - 100   | :heavy_check_mark: | Manual test |
| offset              | int   | 0             | 0 - 2^32  | :heavy_check_mark: | Manual test |
| offset_random       | int   | 0             | 0 - 2^32  | :heavy_check_mark: | Manual test |
| end                 | int   | 0             | -1 - 2^32 | :heavy_check_mark: | Manual test |
| count               | int   | 0             | 0 - 2^32  | :heavy_check_mark: | Manual test |
| loop_mode           | enum  | no_loop       | see spec  | :heavy_check_mark: | Manual test |
| loop_start          | int   | 0             | 0 - 2^32  | :heavy_check_mark: | Manual test |
| loop_end            | int   | 0             | 0 - 2^32  | :heavy_check_mark: | Manual test |

## Instrument settings
|  opcode  | type | default value |     range      |       status       |   tested    |
| -------- | ---- | ------------- | -------------- | ------------------ | ----------- |
| group    | int  | 0             | 0 - 2^32       | :heavy_check_mark: | Manual test |
| off_mode | enum | fast          | fast or normal | :heavy_check_mark: | Manual test |
| off_by   | int  | 0             | 0 - 2^32       | :heavy_check_mark: | Manual test |

## Region logic: key mapping
|      opcode       | type  | default value |  range  |       status       |   tested    |
| ----------------- | ----- | ------------- | ------- | ------------------ | ----------- |
| lokey, hikey, key | int   | 0, 127        | 0 - 127 | :heavy_check_mark: | Unit test |
| lovel, hivel      | float | 0, 127        |         | :heavy_check_mark: | Unit test |

## Region logic: midi conditions
|       opcode       | type  | default value |        range        |       status       |  tested   |
| ------------------ | ----- | ------------- | ------------------- | ------------------ | --------- |
| lochan, hichan     | int   | 1, 16         | 1 - 16              | :heavy_check_mark: | Unit test |
| lobend, hibend     | float | -8192, 8192   | -8192, 8192         | :heavy_check_mark: | Unit test |
| loccN, hiccN       | int   | 0, 127        | 0 - 127             | :heavy_check_mark: | Unit test |
| sw_hikey, sw_lokey | int   | 0, 127        | 0 - 127             | :heavy_check_mark: | Unit test |
| sw_up, sw_down     | int   | unset         | 0 - 127             | :heavy_check_mark: | Unit test |
| sw_last            | int   | unset         | 0 - 127             | :heavy_check_mark: | Unit test |
| sw_previous        | int   | unset         | 0 - 127             | :heavy_check_mark: | Unit test |
| sw_vel             | enum  | current       | current or previous | :heavy_check_mark: | :x:       |
| sw_default         | int   |               | 0 - 127             | :heavy_check_mark: | Unit test |

## Region logic: internal conditions
|        opcode        | type  | default value |  range  |       status       |   tested    |
| -------------------- | ----- | ------------- | ------- | ------------------ | ----------- |
| lorand, hirand       | float | 0, 1          | 0 - 1   | :heavy_check_mark: | Unit test   |
| lochanaft, hichanaft | int   | 0, 127        | 0 - 127 | :heavy_check_mark: | Unit test   |
| lobpm, hibpm         | float | 0, 500        | 0, 500  | :heavy_check_mark: | Unit test   |
| seq_length           | int   | 1             | 1 - 100 | :heavy_check_mark: | Unit test |
| seq_position         | int   | 1             | 1 - 100 | :heavy_check_mark: | Unit test |

## Region logic: triggers
|       opcode       | type | default value |                    range                    |       status       |              tested              |
| ------------------ | ---- | ------------- | ------------------------------------------- | ------------------ | -------------------------------- |
| on_loccN, on_hiccN | int  | 0, 127        | 0 - 127                                     | :heavy_check_mark: | :x:                              |
| trigger            | enum | attack        | attack, release, first, legato, release_key | :heavy_check_mark: | Unit test except `first` and `legato` |

## Performance parameters: Pitch
|     opcode      | type | default value |    range     |       status       |   tested    |
| --------------- | ---- | ------------- | ------------ | ------------------ | ----------- |
| pitch_keycenter | int  | 60 (C4)       | 0 - 127      | :heavy_check_mark: | Manual test |
| pitch_keytrack  | int  | 100           | -1200 - 1200 | :heavy_check_mark: | Manual test |
| pitch_veltrack  | int  | 100           | -9600 - 9600 | :heavy_check_mark: | Manual test |
| pitch_random    | int  | 0             | 0 - 9600     | :heavy_check_mark: | Manual test |
| transpose       | int  | 0             | -127 - 127   | :heavy_check_mark: | Manual test |
| tune            | int  | 0 cents       | -100 - 100   | :heavy_check_mark: | Manual test |

## Performance parameters: Amplifier
|          opcode           | type  | default value |   range    |       status       |   tested    |
| ------------------------- | ----- | ------------- | ---------- | ------------------ | ----------- |
| pan, pan_oncc             | float | 0             | -100 - 100 | :heavy_check_mark: | Manual test |
| position, position_oncc   | float | 0             | -100 - 100 | :heavy_check_mark: | Manual test |
| width, width_oncc         | float | 100           | -100 - 100 | :heavy_check_mark: | Manual test |
| volume, volume_oncc       | float | 0 dB          | -144 - 6   | :x:                | :x:         |
| amplitude, amplitude_oncc | float | 100           | 0 - 100    | :heavy_check_mark: | Manual test |
| amp_keytrack              | float | 0             | -100 - 100 | :x:                | :x:         |
| amp_keycenter             | int   | 60 (C4)       | 0 - 127    | :heavy_check_mark: | Manual test |
| amp_veltrack              | float | 0             | -100 - 100 | :heavy_check_mark: | Manual test |
| amp_velcurve_N            | float | 0             | 0 - 1      | :heavy_check_mark: | Manual test |

## Modulation: Envelope generators

|              opcode               | type  | default value |  range  |       status       |   tested    |
| --------------------------------- | ----- | ------------- | ------- | ------------------ | ----------- |
| ampeg_hold, ampeg_hold_oncc       | float | 0 s           | 0 - 100 | :heavy_check_mark: | Manual test |
| ampeg_delay, ampeg_delay_oncc     | float | 0 s           | 0 - 100 | :heavy_check_mark: | Manual test |
| ampeg_decay, ampeg_decay_oncc     | float | 0 s           | 0 - 100 | :heavy_check_mark: | Manual test |
| ampeg_sustain, ampeg_sustain_oncc | float | 100 %         | 0 - 100 | :heavy_check_mark: | Manual test |
| ampeg_release, ampeg_release_oncc | float | 0 s           | 0 - 100 | :heavy_check_mark: | Manual test |
| ampeg_attack, ampeg_attack_oncc   | float | 0 s           | 0 - 100 | :heavy_check_mark: | Manual test |
| ampeg_start, ampeg_start_oncc     | float | 0 %           | 0 - 100 | :heavy_check_mark: | Manual test |
| ampeg_vel2attack                  | float | 0%            | 0 - 100 | :heavy_check_mark: | :x:         |
| ampeg_vel2decay                   | float | 0%            | 0 - 100 | :heavy_check_mark: | :x:         |
| ampeg_vel2delay                   | float | 0%            | 0 - 100 | :heavy_check_mark: | :x:         |
| ampeg_vel2hold                    | float | 0%            | 0 - 100 | :heavy_check_mark: | :x:         |
| ampeg_vel2release                 | float | 0%            | 0 - 100 | :heavy_check_mark: | :x:         |
| ampeg_vel2sustain                 | float | 0%            | 0 - 100 | :heavy_check_mark: | :x:         |
| ampeg_vel2depth                   | float | 0%            | 0 - 100 | :heavy_check_mark: | :x:         |

## Other capabilities
- :heavy_check_mark: `#define` handling
- :heavy_check_mark: `#include` handling
- Audio formats: :heavy_check_mark: WAV, :heavy_check_mark: FLAC and :heavy_check_mark: Ogg

## Next steps
- Write some more tests for the existing functionality that can be tested
- Pitch-related envelopes and resampler
- Code cleanup
- sfz v2 curves
- EQ
- Filters
- LFOs

