<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- default value for the config XML file path -->
  <xsl:param name="config_filepath">conf.xml</xsl:param>

  <xsl:variable name="dbus_objects" select="/pyxslt" />

  <xsl:template match="/pyxslt">
    <xsl:element name="results">

      <xsl:for-each select="document($config_filepath)/Config/Match">
	<xsl:call-template name="check_this_match">
	  <xsl:with-param name="amatch" select="."/>
	</xsl:call-template>
      </xsl:for-each>

      </xsl:element>
  </xsl:template> 


  <xsl:template name="check_this_match">
    <xsl:param name="amatch"/> <!-- Config Match -->

    <xsl:variable name="current_conf_node" select="."/>

    <xsl:for-each select="$dbus_objects/obj">
      <xsl:call-template name="check_this_match_with_this_obj">
	<xsl:with-param name="amatch" select="$current_conf_node"/>
	<xsl:with-param name="anobj" select="."/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="check_this_match_with_this_obj">
    <xsl:param name="amatch"/> <!-- Config Match -->
    <xsl:param name="anobj"/> <!-- DBUS Object -->

    <xsl:variable name="string1">
      <xsl:for-each select="$amatch/Property">
	<xsl:call-template name="check_this_match_prop">
	  <xsl:with-param name="amatch_prop" select="."/>
	  <xsl:with-param name="anobj" select="$anobj"/>
	</xsl:call-template>
      </xsl:for-each>
    </xsl:variable>

    <xsl:if test="count($amatch/Property) = string-length($string1)">
      <xsl:element name="match">
        <xsl:attribute name="description"><xsl:value-of select="$amatch/@description"/></xsl:attribute>
	<!-- can't find how to make pyxslt set this @, used as a workaround (defined in DbusObject.py) -->
        <!--<xsl:attribute name="dbusObject"><xsl:value-of select="$anobj/@name"/></xsl:attribute>-->
	<xsl:attribute name="dbusObject"><xsl:value-of select="$anobj/item[@key = '_pyxslt_dbus_path']"/></xsl:attribute>
        <xsl:attribute name="propcount"><xsl:value-of select="count($amatch/Property)"/></xsl:attribute>
      </xsl:element>
    </xsl:if>
  </xsl:template>


  <xsl:template name="check_this_match_prop">
    <xsl:param name="amatch_prop"/> <!-- Config Match Prop -->
    <xsl:param name="anobj"/> <!-- DBUS Object -->
    <xsl:variable name="key" select="@id"/>
    <xsl:variable name="value" select="@value"/>
    <!-- if obj match this prop, echoes 1, so in the end we may be able to compare the whole prop of the Match with the total number of character displayed -->
    <xsl:if test="$anobj/item[@key = $key]/text() = $value">1</xsl:if>
      <!--
	  debug with :
	  <xsl:value-of select="./@name"/> match on <xsl:value-of select="$key"/>:<xsl:value-of select="$value"/> (<xsl:value-of select="position()"/>
      -->
  </xsl:template>

</xsl:stylesheet>
