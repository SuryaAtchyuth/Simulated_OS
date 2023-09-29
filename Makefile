CMP = gcc

main = admin.o cpu.o memory.o clock.o idle.o submit.o system.o loader.o paging.o process.o

Output: Batch02_SimOS

Batch02_SimOS: $(main)
	@clear
	@$(CMP)  $(main) swap.o term.o  -pthread -no-pie -w -o  batch2.exe
	@./batch2.exe


clear :
	@rm -rf $(main) batch2.exe

%.o : %.c
	@$(CMP) -w -c $< 
