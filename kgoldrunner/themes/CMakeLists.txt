



########### install files ###############

install( FILES README
	       black-on-white.desktop
	       egypt.desktop
               default.desktop
               kgr_geek.desktop
               nostalgia.desktop
               nostalgia-blues.desktop
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes )

install( FILES 
	default/set.svg default/actors.svg
 	default/gold.ogg default/falling.ogg default/step.wav default/climb.wav
 	default/dig.ogg default/ladder.ogg default/completed.ogg
 	default/death.ogg default/gameover.ogg default/victory.ogg
	DESTINATION 
	${DATA_INSTALL_DIR}/kgoldrunner/themes/default )

install( FILES accessible/black-on-white-set.svg
               accessible/black-on-white-actors.svg
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes/accessible )

install( FILES kgr_geek/set.svgz kgr_geek/actors.svgz
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes/kgr_geek )

install( FILES nostalgia/blue-set.svg nostalgia/blue-actors.svg 
               nostalgia/set.svg nostalgia/actors.svg
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes/nostalgia )

install( FILES egypt/egypt_kgr.svgz egypt/actors.svgz
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes/egypt )

install( FILES kgoldrunner.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )

