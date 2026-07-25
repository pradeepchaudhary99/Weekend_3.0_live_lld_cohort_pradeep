"""
Chain of Responsibility Design Pattern - AI Query Handling Example

A user query passes through a chain of handlers (Cache -> Prompt/Rules ->
RAG -> LLM). Each handler decides if it can resolve the query itself;
if not, it passes the request to the next handler in the chain.
"""

from abc import ABC, abstractmethod
from typing import Optional


# Handler interface
class QueryHandler(ABC):
    def __init__(self):
        self._next_handler: Optional["QueryHandler"] = None

    def set_next(self, handler: "QueryHandler") -> "QueryHandler":
        self._next_handler = handler
        return handler  # allows chaining: h1.set_next(h2).set_next(h3)

    def handle(self, query: str) -> str:
        if self._next_handler:
            return self._next_handler.handle(query)
        return "Sorry, no handler could process this query."


# Concrete handlers (levels of the chain)
class CacheHandler(QueryHandler):
    def __init__(self):
        super().__init__()
        self._cache = {"what is your name": "I'm an AI assistant."}

    def handle(self, query: str) -> str:
        key = query.strip().lower()
        if key in self._cache:
            print("[CacheHandler] hit -> returning cached answer")
            return self._cache[key]
        print("[CacheHandler] miss -> passing to next handler")
        return super().handle(query)


class PromptRulesHandler(QueryHandler):
    """Handles simple, rule-based queries (greetings, small talk) directly."""

    GREETINGS = {"hi", "hello", "hey"}

    def handle(self, query: str) -> str:
        key = query.strip().lower()
        if key in self.GREETINGS:
            print("[PromptRulesHandler] matched greeting -> handling directly")
            return "Hello! How can I help you today?"
        print("[PromptRulesHandler] not a simple prompt -> passing to next handler")
        return super().handle(query)


class RAGHandler(QueryHandler):
    """Handles queries that need retrieval from a knowledge base."""

    KNOWLEDGE_BASE = {
        "refund policy": "Refunds are processed within 5-7 business days.",
        "shipping time": "Standard shipping takes 3-5 business days.",
    }

    def handle(self, query: str) -> str:
        key = query.strip().lower()
        for topic, answer in self.KNOWLEDGE_BASE.items():
            if topic in key:
                print(f"[RAGHandler] found matching document '{topic}' -> retrieving")
                return f"(from docs) {answer}"
        print("[RAGHandler] no relevant document found -> passing to next handler")
        return super().handle(query)


class LLMHandler(QueryHandler):
    """Final fallback: sends the query to a general-purpose LLM."""

    def handle(self, query: str) -> str:
        print("[LLMHandler] generating answer with LLM (fallback)")
        return f"(LLM-generated) Here's my best answer to: '{query}'"


# Client code
if __name__ == "__main__":
    cache = CacheHandler()
    prompt = PromptRulesHandler()
    rag = RAGHandler()
    llm = LLMHandler()

    # Build the chain: cache -> prompt -> rag -> llm
    cache.set_next(prompt).set_next(rag).set_next(llm)

    queries = [
        "what is your name",       # handled by CacheHandler
        "hello",                   # handled by PromptRulesHandler
        "what is your refund policy", # handled by RAGHandler
        "explain quantum computing",  # falls through to LLMHandler
    ]

    for q in queries:
        print(f"\nQuery: {q}")
        print("Answer:", cache.handle(q))