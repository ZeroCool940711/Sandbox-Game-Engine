@echo off
echo ---------------------------------------------------------------------------
echo Compiling All with MOD2X disabled
echo ---------------------------------------------------------------------------
@echo on

@echo off
echo ---------------------------------------------------------------------------
echo Compiling Max preview effects,  MOD2X disabled
echo ---------------------------------------------------------------------------
@echo on
for %%f in (%1) do fxc /Tfx_2_0 /LD /nologo /Fctemp.txt %%f

@echo off
echo --------------------------------------------------------------------------
echo Compiling In-Game effects, with Sky Light Mapping enabled,  MOD2X disabled
echo --------------------------------------------------------------------------
@echo on
for %%f in (%1) do fxc /Tfx_2_0 /LD /nologo /Fctemp.txt /DIN_GAME=1 /DSKY_LIGHT_MAP_ENABLE=1 %%f

@echo off
echo ---------------------------------------------------------------------------
echo Compiling In-Game effects, with Sky Light Mapping disabled,  MOD2X disabled
echo ---------------------------------------------------------------------------
@echo on
for %%f in (%1) do fxc /Tfx_2_0 /LD /nologo /Fctemp.txt /DIN_GAME=1 %%f

@echo off
echo ---------------------------------------------------------------------------
echo Compiling All with MOD2X enabled
echo ---------------------------------------------------------------------------
@echo on

@echo off
echo ---------------------------------------------------------------------------
echo Compiling Max preview effects,  MOD2X enabled
echo ---------------------------------------------------------------------------
@echo on
for %%f in (%1) do fxc /Tfx_2_0 /LD /nologo /Fctemp.txt /DMOD2X=1 %%f

@echo off
echo ---------------------------------------------------------------------------
echo Compiling In-Game effects, with Sky Light Mapping enabled,  MOD2X enabled
echo ---------------------------------------------------------------------------
@echo on
for %%f in (%1) do fxc /Tfx_2_0 /LD /nologo /Fctemp.txt /DMOD2X=1 /DIN_GAME=1 /DSKY_LIGHT_MAP_ENABLE=1 %%f

@echo off
echo ---------------------------------------------------------------------------
echo Compiling In-Game effects, with Sky Light Mapping disabled,  MOD2X enabled
echo ---------------------------------------------------------------------------
@echo on
for %%f in (%1) do fxc /Tfx_2_0 /LD /nologo /Fctemp.txt /DMOD2X=1 /DIN_GAME=1 %%f


@echo off
del temp.txt