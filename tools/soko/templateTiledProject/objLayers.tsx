<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.10.2" name="objLayers" tilewidth="16" tileheight="16" tilecount="11" columns="0">
 <grid orientation="orthogonal" width="1" height="1"/>
 <tile id="0" type="player">
  <properties>
   <property name="gamemode" value="SOKO_CLASSIC"/>
  </properties>
  <image width="16" height="16" source="entitySprites/player16.png"/>
 </tile>
 <tile id="1" type="crate">
  <properties>
   <property name="sticky" type="bool" value="false"/>
   <property name="trail" type="bool" value="false"/>
  </properties>
  <image width="16" height="16" source="entitySprites/crate16.png"/>
 </tile>
 <tile id="2" type="warpinternal">
  <properties>
   <property name="allow_crates" type="bool" value="false"/>
   <property name="hp" type="int" value="0"/>
   <property name="target_id" type="int" value="0"/>
  </properties>
  <image width="16" height="16" source="entitySprites/warpinternal16.png"/>
 </tile>
 <tile id="5" type="warpinternalexit">
  <image width="16" height="16" source="entitySprites/warpinternalexit16.png"/>
 </tile>
 <tile id="3" type="warpexternal">
  <properties>
   <property name="manualIndex" type="bool" value="false"/>
   <property name="target_id" type="int" value="0"/>
  </properties>
  <image width="16" height="16" source="entitySprites/warpexternal16.png"/>
 </tile>
 <tile id="4" type="button">
  <properties>
   <property name="cratePress" type="bool" value="false"/>
   <property name="invertAction" type="bool" value="false"/>
   <property name="numTargets" type="int" value="0"/>
   <property name="playerPress" type="bool" value="false"/>
   <property name="stayDownOnPress" type="bool" value="false"/>
   <property name="target1id" type="int" value="0"/>
   <property name="target2id" type="int" value="0"/>
   <property name="target3id" type="int" value="0"/>
   <property name="target4id" type="int" value="0"/>
  </properties>
  <image width="16" height="16" source="entitySprites/button16.png"/>
 </tile>
 <tile id="6" type="laserEmitUp">
  <properties>
   <property name="emitDirection" value="UP"/>
   <property name="playerMove" type="bool" value="false"/>
  </properties>
  <image width="16" height="16" source="entitySprites/laserEmitUp16.png"/>
 </tile>
 <tile id="7" type="laserReceiveOmni">
  <image width="16" height="16" source="entitySprites/laserReceiveOmni16.png"/>
 </tile>
 <tile id="8" type="laserReceiveUp">
  <properties>
   <property name="emitDirection" value="UP"/>
  </properties>
  <image width="16" height="16" source="entitySprites/laserReceiveUp16.png"/>
 </tile>
 <tile id="9" type="laser90Right">
  <properties>
   <property name="emitDirection" type="bool" value="false"/>
   <property name="playerMove" type="bool" value="false"/>
  </properties>
  <image width="16" height="16" source="entitySprites/laser90Right16.png"/>
 </tile>
 <tile id="10" type="ghostblock">
  <properties>
   <property name="inverted" type="bool" value="false"/>
   <property name="playerMove" type="bool" value="false"/>
  </properties>
  <image width="16" height="16" source="entitySprites/ghostblock16.png"/>
 </tile>
</tileset>
