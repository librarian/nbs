/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

#pragma once
#include <aws/s3/S3_EXPORTS.h>
#include <aws/s3/model/ObjectLockEnabled.h>
#include <aws/s3/model/ObjectLockRule.h>
#include <utility>

namespace Aws
{
namespace Utils
{
namespace Xml
{
  class XmlNode;
} // namespace Xml
} // namespace Utils
namespace S3
{
namespace Model
{

  /**
   * <p>The container element for Object Lock configuration parameters.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ObjectLockConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API ObjectLockConfiguration
  {
  public:
    ObjectLockConfiguration();
    ObjectLockConfiguration(const Aws::Utils::Xml::XmlNode& xmlNode);
    ObjectLockConfiguration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Indicates whether this bucket has an Object Lock configuration enabled.
     * Enable <code>ObjectLockEnabled</code> when you apply
     * <code>ObjectLockConfiguration</code> to a bucket. </p>
     */
    inline const ObjectLockEnabled& GetObjectLockEnabled() const{ return m_objectLockEnabled; }

    /**
     * <p>Indicates whether this bucket has an Object Lock configuration enabled.
     * Enable <code>ObjectLockEnabled</code> when you apply
     * <code>ObjectLockConfiguration</code> to a bucket. </p>
     */
    inline bool ObjectLockEnabledHasBeenSet() const { return m_objectLockEnabledHasBeenSet; }

    /**
     * <p>Indicates whether this bucket has an Object Lock configuration enabled.
     * Enable <code>ObjectLockEnabled</code> when you apply
     * <code>ObjectLockConfiguration</code> to a bucket. </p>
     */
    inline void SetObjectLockEnabled(const ObjectLockEnabled& value) { m_objectLockEnabledHasBeenSet = true; m_objectLockEnabled = value; }

    /**
     * <p>Indicates whether this bucket has an Object Lock configuration enabled.
     * Enable <code>ObjectLockEnabled</code> when you apply
     * <code>ObjectLockConfiguration</code> to a bucket. </p>
     */
    inline void SetObjectLockEnabled(ObjectLockEnabled&& value) { m_objectLockEnabledHasBeenSet = true; m_objectLockEnabled = std::move(value); }

    /**
     * <p>Indicates whether this bucket has an Object Lock configuration enabled.
     * Enable <code>ObjectLockEnabled</code> when you apply
     * <code>ObjectLockConfiguration</code> to a bucket. </p>
     */
    inline ObjectLockConfiguration& WithObjectLockEnabled(const ObjectLockEnabled& value) { SetObjectLockEnabled(value); return *this;}

    /**
     * <p>Indicates whether this bucket has an Object Lock configuration enabled.
     * Enable <code>ObjectLockEnabled</code> when you apply
     * <code>ObjectLockConfiguration</code> to a bucket. </p>
     */
    inline ObjectLockConfiguration& WithObjectLockEnabled(ObjectLockEnabled&& value) { SetObjectLockEnabled(std::move(value)); return *this;}


    /**
     * <p>Specifies the Object Lock rule for the specified object. Enable the this rule
     * when you apply <code>ObjectLockConfiguration</code> to a bucket. Bucket settings
     * require both a mode and a period. The period can be either <code>Days</code> or
     * <code>Years</code> but you must select one. You cannot specify <code>Days</code>
     * and <code>Years</code> at the same time.</p>
     */
    inline const ObjectLockRule& GetRule() const{ return m_rule; }

    /**
     * <p>Specifies the Object Lock rule for the specified object. Enable the this rule
     * when you apply <code>ObjectLockConfiguration</code> to a bucket. Bucket settings
     * require both a mode and a period. The period can be either <code>Days</code> or
     * <code>Years</code> but you must select one. You cannot specify <code>Days</code>
     * and <code>Years</code> at the same time.</p>
     */
    inline bool RuleHasBeenSet() const { return m_ruleHasBeenSet; }

    /**
     * <p>Specifies the Object Lock rule for the specified object. Enable the this rule
     * when you apply <code>ObjectLockConfiguration</code> to a bucket. Bucket settings
     * require both a mode and a period. The period can be either <code>Days</code> or
     * <code>Years</code> but you must select one. You cannot specify <code>Days</code>
     * and <code>Years</code> at the same time.</p>
     */
    inline void SetRule(const ObjectLockRule& value) { m_ruleHasBeenSet = true; m_rule = value; }

    /**
     * <p>Specifies the Object Lock rule for the specified object. Enable the this rule
     * when you apply <code>ObjectLockConfiguration</code> to a bucket. Bucket settings
     * require both a mode and a period. The period can be either <code>Days</code> or
     * <code>Years</code> but you must select one. You cannot specify <code>Days</code>
     * and <code>Years</code> at the same time.</p>
     */
    inline void SetRule(ObjectLockRule&& value) { m_ruleHasBeenSet = true; m_rule = std::move(value); }

    /**
     * <p>Specifies the Object Lock rule for the specified object. Enable the this rule
     * when you apply <code>ObjectLockConfiguration</code> to a bucket. Bucket settings
     * require both a mode and a period. The period can be either <code>Days</code> or
     * <code>Years</code> but you must select one. You cannot specify <code>Days</code>
     * and <code>Years</code> at the same time.</p>
     */
    inline ObjectLockConfiguration& WithRule(const ObjectLockRule& value) { SetRule(value); return *this;}

    /**
     * <p>Specifies the Object Lock rule for the specified object. Enable the this rule
     * when you apply <code>ObjectLockConfiguration</code> to a bucket. Bucket settings
     * require both a mode and a period. The period can be either <code>Days</code> or
     * <code>Years</code> but you must select one. You cannot specify <code>Days</code>
     * and <code>Years</code> at the same time.</p>
     */
    inline ObjectLockConfiguration& WithRule(ObjectLockRule&& value) { SetRule(std::move(value)); return *this;}

  private:

    ObjectLockEnabled m_objectLockEnabled;
    bool m_objectLockEnabledHasBeenSet;

    ObjectLockRule m_rule;
    bool m_ruleHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
