"""
1.创建fastmcp实例
2.创建函数，添加文档
3.@mcp.tool
4.运行服务器
"""
from fastmcp import FastMCP

mcp = FastMCP()

@mcp.tool()
def get_weather(city:str):
    """
    获取对应城市的天气
    :param city:城市
    :return:城市天气的描述
    """
    return f"{city}今天天气晴朗，10度"

@mcp.tool()
def get_year(name:str):
    """
    获取班级上对应同学的年纪
    :param name:姓名
    :return:年纪
    """
    return f"{name}今年18岁"

if __name__ == '__main__':
    mcp.run()