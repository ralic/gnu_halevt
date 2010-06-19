<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
<!-- transform Characteristic having an idref to simple list of all its
     SUB-ELEMENTS from the id-referenced Characteristic
     
     The original ELEMENTS (with an id and no ref) is removed -->

  <xsl:template match="Characteristic">
    <xsl:choose>
      <xsl:when test="@ref">
	<xsl:copy-of select="id(@ref)/*"/>
      </xsl:when>
      <xsl:otherwise> 
	<xsl:apply-templates select="node()"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

<!-- transform Characteristics with an idref to the Property attribute list of
     the id-referenced one
     strip the original (with an id and not ref) if it's INSIDE a Match -->
  <xsl:template match="/Config/Action">
    
  </xsl:template>

  <xsl:template match="Action">
    <xsl:choose>
      <xsl:when test="@ref">
	<xsl:copy-of select="id(@ref)/*"/>
      </xsl:when>
      <xsl:otherwise> 
	<xsl:apply-templates select="node()"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
