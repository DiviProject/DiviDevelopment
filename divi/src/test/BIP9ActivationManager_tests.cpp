#include <test_only.h>

#include <chain.h>
#include <BIP9ActivationManager.h>
#include <BIP9Deployment.h>
#include <ThresholdConditionCache.h>
#include <MockBIP9ActivationTrackerFactory.h>
#include <MockBIP9ActivationStateTracker.h>

using ::testing::Exactly;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::Ref;

struct TestContainer
{
    std::shared_ptr<MockBIP9ActivationTrackerFactory> factory_;
    std::shared_ptr<BIP9ActivationManager> manager_;

    TestContainer(
        ): factory_(std::make_shared<MockBIP9ActivationTrackerFactory>())
        , manager_(std::make_shared<BIP9ActivationManager>(*factory_))
    {
    }
};

BOOST_FIXTURE_TEST_SUITE(BIP9ActivationManager_tests,TestContainer)

BOOST_AUTO_TEST_CASE(willHaveNoBIPsEnabledByDefaultOnConstruction)
{
    BOOST_CHECK(!manager_->networkEnabledBIP("SEGWIT"));
}

BOOST_AUTO_TEST_CASE(willKnowOfNoBIPsByDefault)
{
    BOOST_CHECK(manager_->getBIPStatus("SEGWIT") == BIP9ActivationManager::UNKNOWN_BIP);
}

BOOST_AUTO_TEST_CASE(willRecognizeAnAddedBIP)
{
    BIP9Deployment bip("MySegwitVariant", 1, (int64_t)1500000,(int64_t)1600000,1000,900);

    BOOST_CHECK(manager_->getBIPStatus(bip.deploymentName) == BIP9ActivationManager::UNKNOWN_BIP);
    manager_->addBIP(bip);
    BOOST_CHECK(manager_->getBIPStatus(bip.deploymentName) == BIP9ActivationManager::IN_PROGRESS);
}

BOOST_AUTO_TEST_CASE(willNotAllowAddingBIPsWithOverlappingBits)
{
    BIP9Deployment first("MySegwitVariant", 1, (int64_t)1500000,(int64_t)1600000,1000,900);
    BIP9Deployment second("MyOtherSegwitVariant", 1, (int64_t)1500000,(int64_t)1600000,1000,900);
    
    manager_->addBIP(first);
    BOOST_CHECK(manager_->getBIPStatus(first.deploymentName) == BIP9ActivationManager::IN_PROGRESS);
    
    manager_->addBIP(second);
    BOOST_CHECK(manager_->getBIPStatus(second.deploymentName) == BIP9ActivationManager::UNKNOWN_BIP);
}

BOOST_AUTO_TEST_CASE(willEnableBIPIfChainMeetsSignalingThreshold)
{
    BIP9Deployment firstBIP("MySegwitVariant", 1, (int64_t)1500000,(int64_t)1600000,1000,900);
    const CBlockIndex* chainTip = new CBlockIndex();

    ON_CALL(*factory_, create(_,_))
        .WillByDefault(
            Invoke( 
                [chainTip](const BIP9Deployment& a, ThresholdConditionCache& b)-> I_BIP9ActivationStateTracker*
                {
                    auto mock = new MockBIP9ActivationStateTracker();
                    ON_CALL(*mock,getLastCachedStatePriorToBlockIndex(chainTip))
                        .WillByDefault(
                            Return(ThresholdState::ACTIVE)
                        );
                    return mock;
                }
            )
        );
    
    manager_->addBIP(firstBIP);
    
    manager_->update(chainTip);
    BOOST_CHECK(manager_->networkEnabledBIP(firstBIP.deploymentName));
   
}

BOOST_AUTO_TEST_SUITE_END()