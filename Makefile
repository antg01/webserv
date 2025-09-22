# **************************************************************************** #
#                                   Webserv                                    #
# **************************************************************************** #

NAME        = webserv

CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -Iinclude

SRC_DIR     = src
OBJ_DIR     = obj

SRC         = $(SRC_DIR)/main.cpp \
              $(SRC_DIR)/config/ConfigParser.cpp \
              $(SRC_DIR)/io/createSocket.cpp 

OBJ         = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Colors
GREEN       = \033[0;32m
RED         = \033[0;31m
RESET       = \033[0m

all: $(NAME)

$(NAME): $(OBJ)
	@$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "$(GREEN)[OK]$(RESET) Build complete: $(NAME)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(RED)[CLEAN]$(RESET) Objects removed"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)[FCLEAN]$(RESET) Executable removed"

re: fclean all

.PHONY: all clean fclean re

