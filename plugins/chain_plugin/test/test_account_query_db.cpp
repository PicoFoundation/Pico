#define BOOST_TEST_MODULE account_query_db
#include <picoio/chain/permission_object.hpp>
#include <boost/test/unit_test.hpp>
#include <picoio/testing/tester.hpp>
#include <picoio/chain/types.hpp>
#include <picoio/chain/block_state.hpp>
#include <picoio/chain_plugin/account_query_db.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace picoio;
using namespace picoio::chain;
using namespace picoio::testing;
using namespace picoio::chain_apis;

using params  = account_query_db::get_accounts_by_authorizers_params;
using results = account_query_db::get_accounts_by_authorizers_result;

bool find_account_name(results rst, account_name name){
    for (const auto acc : rst.accounts){
        if (acc.account_name == name){
            return true;
        }
    }
    return false;
}
bool find_account_auth(results rst, account_name name, permission_name perm){
    for (const auto acc : rst.accounts){
        if (acc.account_name == name && acc.permission_name == perm)
            return true;
    }
    return false;
}

BOOST_AUTO_TEST_SUITE(account_query_db_tests)

BOOST_FIXTURE_TEST_CASE(newaccount_test, TESTER) { try {

	// instantiate an account_query_db
	auto aq_db = account_query_db(*control);

    //link aq_db to the `accepted_block` signal on the controller
	auto c2 = control->accepted_block.connect([&](const block_state_ptr& blk) {
        aq_db.commit_block( blk);
	});

	produce_blocks(10);

	account_name tester_account = N(tester);
	const auto trace_ptr =  create_account(tester_account);
	aq_db.cache_transaction_trace(trace_ptr);
	produce_block();

	params pars;
	pars.keys.emplace_back(get_public_key(tester_account, "owner"));
	const auto results = aq_db.get_accounts_by_authorizers(pars);

	BOOST_TEST_REQUIRE(find_account_name(results, tester_account) == true);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(updateauth_test, TESTER) { try {

    // instantiate an account_query_db
    auto aq_db = account_query_db(*control);

    //link aq_db to the `accepted_block` signal on the controller
    auto c = control->accepted_block.connect([&](const block_state_ptr& blk) {
        aq_db.commit_block( blk);
    });

    produce_blocks(10);

	const auto& tester_account = N(tester);
	const string role = "first";
	produce_block();
	create_account(tester_account);

	const auto trace_ptr = push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
			("account", tester_account)
			("permission", N(role))
			("parent", "active")
			("auth",  authority(get_public_key(tester_account, role), 5))
	);
	aq_db.cache_transaction_trace(trace_ptr);
	produce_block();

	params pars;
	pars.keys.emplace_back(get_public_key(tester_account, role));
	const auto results = aq_db.get_accounts_by_authorizers(pars);

	BOOST_TEST_REQUIRE(find_account_auth(results, tester_account, N(role)) == true);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

