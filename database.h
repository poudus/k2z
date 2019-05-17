
#include <libpq-fe.h>


typedef struct
{
double	lambda_decay, opp_decay, rating, spread, zeta;
int	pid, max_moves, depth;
} PLAYER_PARAMETERS;


typedef struct
{
	char   move[4], code[32];
	double score, ratio;
	int    id, depth, parent, sid, visits;
} MCTS;


int getldate(time_t tt);
int getlnow();
unsigned long long getCurrentUTS();

PGconn *pgOpenConn(const char *pdbname, const char *pdbusername, const char *pdbpassword, char *buffer_error);
void pgCloseConn(PGconn *conn);

long long pgGetSequo(PGconn *pgConn, const char *psequo);
bool pgExec(PGconn *pgConn, int *pnbc, const char *pCommand);
PGresult *pgQuery(PGconn *pgConn, const char *pQuery);
bool pgExecFormat(PGconn *pgConn, int *pnbc, char *szFormat, ...);

int pgGetCount(PGconn *pgConn, const char *ptable, const char *pwhere);
int pgGetMax(PGconn *pgConn, const char *pColumn, const char *ptable, const char *pwhere);
double pgGetSum(PGconn *pgConn, const char *pColumn, const char *ptable, const char *pwhere);
double pgGetDouble(PGconn *pgConn, const char *pQuery);

char *formatTS(long long ts, char *buffer);

double EloExpectedResult(double r1, double r2);
double EloDifference(double p);
bool UpdateRatings(PGconn *pgConn, int hp, int vp, char winner, double coef);

int insertGame(PGconn *pgConn, int hp, int vp, char *moves, char winner, char reason, int duration, int ht, int vt);
bool LoadGame(PGconn *pgConn, int game, char *moves);
bool LoadPlayerParameters(PGconn *pgConn, int player, PLAYER_PARAMETERS *pp);

bool UpdatePlayerWin(PGconn *pgConn, int player, double gain);
bool UpdatePlayerLoss(PGconn *pgConn, int player, double loss);
bool UpdatePlayerDraw(PGconn *pgConn, int player, double gl);

bool RegisterLive(PGconn *pgConn, int channel, int hp);
bool JoinLive(PGconn *pgConn, int channel, int vp);
void PrintLive(PGconn *pgConn, int channel);
bool DeleteLive(PGconn *pgConn, int channel);
bool CheckLive(PGconn *pgConn, int channel, char orientation, char *last_move, char *moves);
bool PlayLive(PGconn *pgConn, int channel, char orientation, char *move, char *moves, char *signature);
bool ResignLive(PGconn *pgConn, int channel, char winner, char reason);
bool CheckResign(PGconn *pgConn, int channel, char orientation, char *moves);
bool WinLive(PGconn *pgConn, int channel, char winner, char reason);
int WaitLive(PGconn *pgConn, int channel);
bool UpdateLiveSignature(PGconn *pgConn, int channel, char *signature);
bool LivePlayers(PGconn *pgConn, int channel, int* hp, int* vp);

// MCTS
int insert_mcts(PGconn *pgConn, int depth, int parent,
                      int sid, const char *move, const char *code,
                      int visits, double score);

bool update_mcts(PGconn *pgConn, int id, double score, int visits);

bool best_ucb_child(PGconn *pgConn, MCTS *parent, double exploration, MCTS* best_child);
bool find_mcts_node(PGconn *pgConn, int id, MCTS* node);
int mcts_child_nodes(PGconn *pgConn, int size, const char* parameters, MCTS *nodes);
int mcts_children(PGconn *pgConn, int parent, MCTS *nodes);
int search_mcts_node(PGconn *pgConn, const char* code, MCTS* node);
int find_children(PGconn *pgConn, int parent, int sid, MCTS *nodes);










