using namespace eosio;
using namespace eosiosystem;
using namespace guardian;
using std::string;

namespace validation {
    // validate blacklist
    void validate_blacklist(name code, name user, name to) {
        blacklist_table b(code, user.value);
        auto itr = b.find(to.value);
        eosio_assert(itr == b.end(), "account in blacklist");
    }

    // delete expired records
    void delete_records(name code, name user, const std::vector<uint64_t>& ids=std::vector<uint64_t>()) {
        txrecord_table t(code, user.value);
        
        for(int i = 0; i < ids.size(); i++) {
            uint64_t id = ids[i];
            //print(" | id:", id);
            auto itr = t.find(id);
            if(itr != t.end()) {
                t.erase(itr);
            }
        }
    }

    // get total transfer record
    asset get_cap_used(name code, name user, name to, asset quantity, uint64_t duration) {
        txrecord_table t(code, user.value);
        
        auto idx = t.get_index<"to"_n>();
        asset used{0, EOS_SYMBOL};
        
        std::vector<uint64_t> to_delete_ids;
        
        auto first = idx.lower_bound(to.value);
        auto last = idx.upper_bound(to.value);
        auto n = now();
        
        while(first != last && first != idx.end()) {
            if((duration * 60 + first->created_at) > n) {
                // not expired
                used += first->quantity;
            } else {
                // expired record should be deleted
                to_delete_ids.emplace_back(first->id);
            }
            first++;
        }
        // add quantity to used
        used += quantity;

        // delete expired orders
        delete_records(code, user, to_delete_ids);

        return used;
    }

    // validate transfer
    void validate_transfer(name code, name user, name to, asset quantity) {
        whitelist_table w(code, user.value);
        asset cap_total{0, EOS_SYMBOL};
        asset cap_tx{0, EOS_SYMBOL};
        uint64_t duration;
        auto itr = w.find(to.value);

        if(itr != w.end()) {
            cap_total = itr->cap_total;
            cap_tx = itr->cap_tx;
            duration = itr->duration;
        } else {
            settings_table s(code, user.value);
            auto it = s.find(user.value);
            cap_total = it->cap_total;
            cap_tx = it->cap_tx;
            duration = it->duration;
        }
        
        eosio_assert(quantity <= cap_tx, "cap_tx exceeded!");
        asset cap_used = get_cap_used(code, user, to, quantity, duration);
        //print("cap_used:", cap_used.amount);
        //print("cap_total:", cap_total.amount);
        eosio_assert(cap_used <= cap_total, "cap_total exceeded!");
    }

    // validate user
    void validate_user(name code, name user) {
        users_table s(code, user.value);
        auto idx = s.find(user.value);
        eosio_assert(idx != s.end(), "User does not exist!");
    }
    // get user status
    uint64_t get_user_status(name code, name user) {
        uint64_t status = USER_STATUS_EFFECTIVE;
        users_table s(code, user.value);
        auto idx = s.find(user.value);
        auto n = now();
        if((idx->duration * 60 + idx->created_at) < n) {
            //user subscription expired;
            status = USER_STATUS_EXPIRED;
        }
        return status;
    }
}
