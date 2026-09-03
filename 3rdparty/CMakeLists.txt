cmake_minimum_required(VERSION 3.14.1)


######################################################################
# hcom
######################################################################
SET(HCOM_LIBS_DIR /usr/lib64 PARENT_SCOPE)
SET(HCOM_C_INCLUDE_DIR /usr/include/hcom/capi PARENT_SCOPE)
SET(HCOM_INCLUDE_DIR /usr/include/hcom PARENT_SCOPE)

MACRO(TARGET_HCOM target)
    TARGET_INCLUDE_DIRECTORIES(${target} SYSTEM PUBLIC ${HCOM_C_INCLUDE_DIR})
    TARGET_INCLUDE_DIRECTORIES(${target} SYSTEM PUBLIC ${HCOM_INCLUDE_DIR})
    TARGET_LINK_DIRECTORIES(${target} PUBLIC ${HCOM_LIBS_DIR})
    TARGET_LINK_LIBRARIES(${target} PUBLIC hcom)
ENDMACRO()

MACRO(TARGET_HCOM_HEADER_ONLY target)
    TARGET_INCLUDE_DIRECTORIES(${target} SYSTEM PUBLIC ${HCOM_C_INCLUDE_DIR})
    TARGET_INCLUDE_DIRECTORIES(${target} SYSTEM PUBLIC ${HCOM_INCLUDE_DIR})
ENDMACRO()

######################################################################
# libboundscheck
######################################################################
SET(SECUREC_INCLUDE_DIR /usr/include/ PARENT_SCOPE)
MACRO(TARGET_SECUREC_SHARE target)
    TARGET_INCLUDE_DIRECTORIES(${target} SYSTEM PUBLIC ${SECUREC_INCLUDE_DIR})
    TARGET_LINK_LIBRARIES(${target} PUBLIC boundscheck)
ENDMACRO()

######################################################################
# obmm库 so,以下路径为obmm.rpm安装后的路径
######################################################################
MACRO(TARGET_OBMM_HEADER_ONLY target)
    target_include_directories(${target} SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/obmm_ub)
    target_include_directories(${target} SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/obmm_ub/linux)
ENDMACRO()
