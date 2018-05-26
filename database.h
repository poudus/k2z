
#include <libpq-fe.h>


typedef struct
{
double	lambda_decay, opp_decay;
int	pid, max_moves, depth;
} PLAYER_PARAMETERS;


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

int insertGame(PGconn *pgConn, int hp, int vp, char *moves, char winner, char reason, int duration, int ht, int vt);
bool LoadPlayerParameters(PGconn *pgConn, int player, PLAYER_PARAMETERS *pp);

bool UpdatePlayerWin(PGconn *pgConn, int player, double gain);
bool UpdatePlayerLoss(PGconn *pgConn, int player, double loss);
