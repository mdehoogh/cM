cmake_minimum_required(VERSION 3.10)

# set the project name, version and used language
project(cM VERSION 0.1.7 LANGUAGES C)

# remember the build time
string(TIMESTAMP BUILD_TIME "%d-%m-%Y %H:%M:%S")

# do we need this??
configure_file(${CMAKE_SOURCE_DIR}/src/MConfig.h.in ${CMAKE_SOURCE_DIR}/src/MConfig.h)

# instead of adding the libraries used, we assume that the static tommath and mpdec libraries are present in the source root created earlier
# with make in tommath and configure && make in mpdecimal-2.4.2, which renders the following obsolete

## # add the tommath subdirectory
## add_subdirectory(libtommath)
##
## # add the mpdec subdirectory
## add_subdirectory(mpdecimal-2.4.2/libmpdec)

# add the executable, pointing to the source files in reverse order
add_executable(M
	${CMAKE_SOURCE_DIR}/src/M.c 
	${CMAKE_SOURCE_DIR}/src/Msession.c
	${CMAKE_SOURCE_DIR}/src/Mcolors.c
	${CMAKE_SOURCE_DIR}/src/Mshell.c
	${CMAKE_SOURCE_DIR}/src/Menvironment.c
	${CMAKE_SOURCE_DIR}/src/Mfunctions.c
	${CMAKE_SOURCE_DIR}/src/Mlocale.c
	${CMAKE_SOURCE_DIR}/src/Mmatrix.c
	${CMAKE_SOURCE_DIR}/src/Moperations.c
	${CMAKE_SOURCE_DIR}/src/Mlist.c
	${CMAKE_SOURCE_DIR}/src/Marray.c
	${CMAKE_SOURCE_DIR}/src/Miterator.c
	${CMAKE_SOURCE_DIR}/src/Mtime.c
	${CMAKE_SOURCE_DIR}/src/Msystem.c
	${CMAKE_SOURCE_DIR}/src/Mvalue.c
	${CMAKE_SOURCE_DIR}/src/Mdecimal.c
	${CMAKE_SOURCE_DIR}/src/Mrational.c
	${CMAKE_SOURCE_DIR}/src/Mbiginteger.c
	${CMAKE_SOURCE_DIR}/src/Mexecution.c
	${CMAKE_SOURCE_DIR}/src/Mmemory.c
	${CMAKE_SOURCE_DIR}/src/Mtoken.c
	${CMAKE_SOURCE_DIR}/src/Msettings.c
	${CMAKE_SOURCE_DIR}/src/Mjson.c
	${CMAKE_SOURCE_DIR}/src/Mstring.c
	${CMAKE_SOURCE_DIR}/src/Mchars.c
	${CMAKE_SOURCE_DIR}/src/Malloc.c
	${CMAKE_SOURCE_DIR}/src/Mmessage.c
	${CMAKE_SOURCE_DIR}/src/Moutput.c
	${CMAKE_SOURCE_DIR}/src/Mmodule.c
)

target_link_libraries(M PUBLIC ${CMAKE_SOURCE_DIR}/libtommath.a ${CMAKE_SOURCE_DIR}/libmpdec.a)
#replacing: target_link_libraries(M PUBLIC tommath mpdec)

# at the end we need the following when using a configure_file
##target_include_directories(M PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tommath ${CMAKE_SOURCE_DIR}/mpdecimal-2.4.2/libmpdec)
###replacing: target_include_directories(M PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/libtommath")
