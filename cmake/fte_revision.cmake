
find_package(Git)

if(Git_FOUND AND EXISTS ${PROJECT_SOURCE_DIR}/.git)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --always --long --dirty
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		OUTPUT_VARIABLE git_commit
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} log -1 --format=%cs
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		OUTPUT_VARIABLE git_date
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} branch --show-current
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		OUTPUT_VARIABLE git_branch
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --is-shallow-repository
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		OUTPUT_VARIABLE git_is_shallow
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(git_is_shallow)
		message(STATUS "shallow clone prevents calculation of revision number.")
		set(git_revision "git-${git_commit}") #if its a shallow clone then we can't count commits properly so don't know what revision we actually are.
	else()
		execute_process(
			COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
			WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
			OUTPUT_VARIABLE git_revision
			ERROR_QUIET
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		math(EXPR git_revision "${git_revision} + 29") #not all svn commits managed to appear im the git repo, so we have a small bias to keep things consistent.
		if(git_branch STREQUAL "main" OR git_branch STREQUAL "")
			set(git_revision "${git_revision}-git-${git_commit}")
		else()
			set(git_revision "${git_branch}-${git_revision}-git-${git_commit}") #weird branches get a different form of revision, to reduce confusion.
		endif()
	endif()
	message(STATUS "FTE GIT ${git_branch} Revision ${git_revision}, ${git_date}")
	list(APPEND FTE_COMMON_DEFINITIONS SVNREVISION=${git_revision})
	list(APPEND FTE_COMMON_DEFINITIONS SVNDATE=${git_date})
	list(APPEND FTE_COMMON_DEFINITIONS FTE_BRANCH=${git_branch})
endif()
