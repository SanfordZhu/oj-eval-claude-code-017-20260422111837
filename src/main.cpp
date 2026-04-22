#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>

using namespace std;

// Simple persistent user storage without STL containers (except string)
struct User {
    char username[21];
    char password[31];
    char name[64];
    char mail[31];
    int privilege;
    bool logged; // not persisted
};

static const int MAX_USERS = 50000;
static User users[MAX_USERS];
static int user_count = 0;

const char *USER_FILE = "users.dat";

void load_users() {
    FILE *f = fopen(USER_FILE, "rb");
    if (!f) return;
    uint32_t cnt = 0;
    if (fread(&cnt, sizeof(uint32_t), 1, f) != 1) { fclose(f); return; }
    if (cnt > MAX_USERS) cnt = MAX_USERS;
    for (uint32_t i = 0; i < cnt; ++i) {
        User u; size_t r = fread(&u, sizeof(User) - sizeof(bool), 1, f); // exclude logged
        if (r != 1) break;
        u.logged = false;
        users[i] = u;
    }
    user_count = (int)cnt;
    fclose(f);
}

void save_users() {
    FILE *f = fopen(USER_FILE, "wb");
    if (!f) return;
    uint32_t cnt = (uint32_t)user_count;
    fwrite(&cnt, sizeof(uint32_t), 1, f);
    for (int i = 0; i < user_count; ++i) {
        // write without logged
        fwrite(users + i, sizeof(User) - sizeof(bool), 1, f);
    }
    fclose(f);
}

int find_user(const string &uname) {
    for (int i = 0; i < user_count; ++i) {
        if (strcmp(users[i].username, uname.c_str()) == 0) return i;
    }
    return -1;
}

static inline void set_str(char *dst, size_t cap, const string &src) {
    size_t n = src.size();
    if (n >= cap) n = cap - 1;
    memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

struct Params { // hold up to 16 params
    char keys[16];
    string vals[16];
    int n;
};

Params parse_params(const string &line) {
    Params p; p.n = 0;
    // tokenize by spaces
    int i = 0; int L = (int)line.size();
    while (i < L) {
        while (i < L && (line[i] == ' ' || line[i] == '\t')) ++i;
        if (i >= L) break;
        int j = i;
        while (j < L && line[j] != ' ' && line[j] != '\t') ++j;
        string tok = line.substr(i, j - i);
        i = j;
        if (!tok.empty() && tok[0] == '-' && tok.size() == 2 && p.n < 16) {
            // read next as value
            while (i < L && (line[i] == ' ' || line[i] == '\t')) ++i;
            int k = i;
            while (k < L && line[k] != ' ' && line[k] != '\t') ++k;
            string val;
            if (k > i) val = line.substr(i, k - i);
            p.keys[p.n] = tok[1]; p.vals[p.n] = val; p.n++;
            i = k;
        }
    }
    return p;
}

bool has_key(const Params &p, char k) {
    for (int i = 0; i < p.n; ++i) if (p.keys[i] == k) return true;
    return false;
}
string get_val(const Params &p, char k) {
    for (int i = 0; i < p.n; ++i) if (p.keys[i] == k) return p.vals[i];
    return string();
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    load_users();

    string line;
    while (true) {
        if (!std::getline(cin, line)) break;
        if (line.empty()) continue;
        // extract command (first token)
        size_t sp = line.find(' ');
        string cmd = (sp == string::npos) ? line : line.substr(0, sp);
        string rest = (sp == string::npos) ? string() : line.substr(sp + 1);

        if (cmd == "exit") {
            cout << "bye\n";
            break;
        } else if (cmd == "clean") {
            // reset users
            user_count = 0;
            // delete file
            FILE *f = fopen(USER_FILE, "wb");
            if (f) { uint32_t zero = 0; fwrite(&zero, sizeof(uint32_t), 1, f); fclose(f); }
            cout << 0 << '\n';
        } else if (cmd == "add_user") {
            Params p = parse_params(rest);
            string u = get_val(p,'u');
            string pw = get_val(p,'p');
            string nm = get_val(p,'n');
            string ml = get_val(p,'m');
            string cg = get_val(p,'g');
            string cu = get_val(p,'c');

            if (user_count == 0) {
                if (find_user(u) != -1) { cout << -1 << '\n'; continue; }
                if (user_count >= MAX_USERS) { cout << -1 << '\n'; continue; }
                User nu{}; set_str(nu.username,21,u); set_str(nu.password,31,pw);
                set_str(nu.name,64,nm); set_str(nu.mail,31,ml); nu.privilege = 10; nu.logged=false;
                users[user_count++] = nu; save_users(); cout << 0 << '\n';
            } else {
                int cidx = find_user(cu);
                if (cidx == -1 || !users[cidx].logged) { cout << -1 << '\n'; continue; }
                int g = 0; if (!cg.empty()) { g = atoi(cg.c_str()); } else { cout << -1 << '\n'; continue; }
                if (g >= users[cidx].privilege) { cout << -1 << '\n'; continue; }
                if (find_user(u) != -1) { cout << -1 << '\n'; continue; }
                if (user_count >= MAX_USERS) { cout << -1 << '\n'; continue; }
                User nu{}; set_str(nu.username,21,u); set_str(nu.password,31,pw);
                set_str(nu.name,64,nm); set_str(nu.mail,31,ml); nu.privilege = g; nu.logged=false;
                users[user_count++] = nu; save_users(); cout << 0 << '\n';
            }
        } else if (cmd == "login") {
            Params p = parse_params(rest);
            string u = get_val(p,'u');
            string pw = get_val(p,'p');
            int idx = find_user(u);
            if (idx == -1) { cout << -1 << '\n'; continue; }
            if (users[idx].logged) { cout << -1 << '\n'; continue; }
            if (strcmp(users[idx].password, pw.c_str()) != 0) { cout << -1 << '\n'; continue; }
            users[idx].logged = true;
            cout << 0 << '\n';
        } else if (cmd == "logout") {
            Params p = parse_params(rest);
            string u = get_val(p,'u');
            int idx = find_user(u);
            if (idx == -1) { cout << -1 << '\n'; continue; }
            if (!users[idx].logged) { cout << -1 << '\n'; continue; }
            users[idx].logged = false;
            cout << 0 << '\n';
        } else if (cmd == "query_profile") {
            Params p = parse_params(rest);
            string cu = get_val(p,'c');
            string u = get_val(p,'u');
            int cidx = find_user(cu);
            int uidx = find_user(u);
            if (cidx == -1 || uidx == -1 || !users[cidx].logged) { cout << -1 << '\n'; continue; }
            if (!(users[cidx].privilege > users[uidx].privilege || cidx == uidx)) { cout << -1 << '\n'; continue; }
            cout << users[uidx].username << ' ' << users[uidx].name << ' ' << users[uidx].mail << ' ' << users[uidx].privilege << '\n';
        } else if (cmd == "modify_profile") {
            Params p = parse_params(rest);
            string cu = get_val(p,'c');
            string u = get_val(p,'u');
            int cidx = find_user(cu);
            int uidx = find_user(u);
            if (cidx == -1 || uidx == -1 || !users[cidx].logged) { cout << -1 << '\n'; continue; }
            if (!(users[cidx].privilege > users[uidx].privilege || cidx == uidx)) { cout << -1 << '\n'; continue; }
            if (has_key(p,'g')) {
                int g = atoi(get_val(p,'g').c_str());
                if (g >= users[cidx].privilege) { cout << -1 << '\n'; continue; }
                users[uidx].privilege = g;
            }
            if (has_key(p,'p')) set_str(users[uidx].password,31,get_val(p,'p'));
            if (has_key(p,'n')) set_str(users[uidx].name,64,get_val(p,'n'));
            if (has_key(p,'m')) set_str(users[uidx].mail,31,get_val(p,'m'));
            save_users();
            cout << users[uidx].username << ' ' << users[uidx].name << ' ' << users[uidx].mail << ' ' << users[uidx].privilege << '\n';
        } else if (cmd == "add_train" || cmd == "release_train" || cmd == "query_train" || cmd == "delete_train" ||
                   cmd == "query_ticket" || cmd == "query_transfer" || cmd == "buy_ticket" || cmd == "query_order" ||
                   cmd == "refund_ticket") {
            cout << -1 << '\n';
        } else {
            cout << -1 << '\n';
        }
    }

    return 0;
}
