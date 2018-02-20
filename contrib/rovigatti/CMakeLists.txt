SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

ADD_CUSTOM_TARGET(rovigatti
	DEPENDS ConstructwisePressure MGAnalysis AOInteraction RadialDensityProfile MGInteraction GenericGrByInsertion YasutakaAnalysis VoidPercolation Remoteness CPAnalysis Widom LevyDelta LevyInteraction CPMixtureInteraction StarrInteraction NathanInteraction mWInteraction ManfredoInteraction GraftedInteraction FSInteraction TSPAnalysis StarrConf SPBAnalysis PatchyToMgl Diblock ConstructwiseBonds DiblockComs EmptyVolume FSConf GrByInsertion Gyradius PatchyToMgl  
)

SET(CMAKE_SHARED_LIBRARY_PREFIX "")

# Observables
ADD_LIBRARY(Diblock SHARED EXCLUDE_FROM_ALL src/Observables/DiblockGr.cpp)
ADD_LIBRARY(ConstructwiseBonds SHARED EXCLUDE_FROM_ALL src/Observables/ConstructwiseBonds.cpp)
ADD_LIBRARY(DiblockComs SHARED EXCLUDE_FROM_ALL src/Observables/DiblockComs.cpp)
ADD_LIBRARY(EmptyVolume SHARED EXCLUDE_FROM_ALL src/Observables/EmptyVolume.cpp)
ADD_LIBRARY(FSConf SHARED EXCLUDE_FROM_ALL src/Observables/FSConf.cpp)
ADD_LIBRARY(GrByInsertion SHARED EXCLUDE_FROM_ALL src/Observables/GrByInsertion.cpp)
ADD_LIBRARY(Gyradius SHARED EXCLUDE_FROM_ALL src/Observables/Gyradius.cpp)
ADD_LIBRARY(PatchyToMgl SHARED EXCLUDE_FROM_ALL src/Observables/PatchyToMgl.cpp)
ADD_LIBRARY(SPBAnalysis SHARED EXCLUDE_FROM_ALL src/Observables/SPBAnalysis.cpp)
ADD_LIBRARY(StarrConf SHARED EXCLUDE_FROM_ALL src/Observables/StarrConf.cpp)
ADD_LIBRARY(TSPAnalysis SHARED EXCLUDE_FROM_ALL src/Observables/TSPAnalysis.cpp)
ADD_LIBRARY(Widom SHARED EXCLUDE_FROM_ALL src/Observables/Widom.cpp)
ADD_LIBRARY(LevyDelta SHARED EXCLUDE_FROM_ALL src/Observables/LevyDelta.cpp)
ADD_LIBRARY(CPAnalysis SHARED EXCLUDE_FROM_ALL src/Observables/CPAnalysis.cpp)
ADD_LIBRARY(Remoteness SHARED EXCLUDE_FROM_ALL src/Observables/Remoteness.cpp)
ADD_LIBRARY(VoidPercolation SHARED EXCLUDE_FROM_ALL src/Observables/VoidPercolation.cpp)
ADD_LIBRARY(YasutakaAnalysis SHARED EXCLUDE_FROM_ALL src/Observables/YasutakaAnalysis.cpp)
ADD_LIBRARY(GenericGrByInsertion SHARED EXCLUDE_FROM_ALL src/Observables/GenericGrByInsertion.cpp)
ADD_LIBRARY(RadialDensityProfile SHARED EXCLUDE_FROM_ALL src/Observables/RadialDensityProfile.cpp)
ADD_LIBRARY(MGAnalysis SHARED EXCLUDE_FROM_ALL src/Observables/MGAnalysis.cpp)
ADD_LIBRARY(ConstructwisePressure SHARED EXCLUDE_FROM_ALL src/Observables/ConstructwisePressure)

# Interactions
ADD_LIBRARY(FSInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/FSInteraction.cpp)
ADD_LIBRARY(GraftedInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/GraftedInteraction.cpp)
ADD_LIBRARY(ManfredoInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/ManfredoInteraction.cpp)
ADD_LIBRARY(mWInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/mWInteraction.cpp)
ADD_LIBRARY(NathanInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/NathanInteraction.cpp)
ADD_LIBRARY(StarrInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/StarrInteraction.cpp)
ADD_LIBRARY(CPMixtureInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/CPMixtureInteraction.cpp)
ADD_LIBRARY(LevyInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/LevyInteraction.cpp)
ADD_LIBRARY(MGInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/MGInteraction.cpp)
ADD_LIBRARY(AOInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/AOInteraction.cpp)

FIND_PACKAGE(PkgConfig)
IF(PkgConfig_FOUND)
	PKG_CHECK_MODULES(GSL gsl)
	IF(GSL_FOUND)
		ADD_LIBRARY(NathanNeighs SHARED EXCLUDE_FROM_ALL src/Observables/NathanNeighs.cpp)
		TARGET_LINK_LIBRARIES(NathanNeighs ${GSL_LIBRARIES})
	
		ADD_LIBRARY(NathanStarInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/NathanStarInteraction.cpp)
		TARGET_LINK_LIBRARIES(NathanStarInteraction ${GSL_LIBRARIES})
	
		ADD_DEPENDENCIES(rovigatti NathanNeighs NathanStarInteraction)
	ENDIF(GSL_FOUND)
ENDIF(PkgConfig_FOUND)

IF(CUDA)
	CUDA_ADD_LIBRARY(CUDAFSInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/CUDAFSInteraction.cu src/Interactions/FSInteraction.cpp)
	CUDA_ADD_LIBRARY(CUDAmWInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/CUDAmWInteraction.cu src/Interactions/mWInteraction.cpp)
	CUDA_ADD_LIBRARY(CUDAStarrInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/CUDAStarrInteraction.cu src/Interactions/StarrInteraction.cpp)
	CUDA_ADD_LIBRARY(CUDALevyInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/CUDALevyInteraction.cu src/Interactions/LevyInteraction.cpp)
	CUDA_ADD_LIBRARY(CUDACPMixtureInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/CUDACPMixtureInteraction.cu src/Interactions/CPMixtureInteraction.cpp)
	CUDA_ADD_LIBRARY(CUDAMGInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/CUDAMGInteraction.cu src/Interactions/MGInteraction.cpp)
	CUDA_ADD_LIBRARY(CUDAAOInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/CUDAAOInteraction.cu src/Interactions/AOInteraction.cpp)
	
	ADD_DEPENDENCIES(rovigatti CUDAAOInteraction CUDAMGInteraction CUDACPMixtureInteraction CUDAFSInteraction CUDAmWInteraction CUDAStarrInteraction CUDALevyInteraction)
	
	IF(GSL_FOUND)
		CUDA_ADD_LIBRARY(CUDANathanStarInteraction SHARED EXCLUDE_FROM_ALL src/Interactions/CUDANathanStarInteraction.cu src/Interactions/NathanStarInteraction.cpp)
		TARGET_LINK_LIBRARIES(CUDANathanStarInteraction ${GSL_LIBRARIES})
		ADD_DEPENDENCIES(rovigatti CUDANathanStarInteraction)
	ENDIF(GSL_FOUND)
ENDIF(CUDA)