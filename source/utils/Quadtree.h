#pragma once

#include <cassert>
#include <vector>
#include <array>
#include <memory>

#include "../utils/MathUtils.h"

namespace WorldAssistant
{

static constexpr std::size_t QUADTREE_THRESHOLD = 16;
static constexpr std::size_t QUADTREE_MAX_DEPTH = 8;

struct QuadtreeValue
{
	Rect box_;
};

class Quadtree
{
public:
	Quadtree(const Rect& box) :
        box_(box),
        root_(std::make_unique<Node>())
	{
	}

	void Add(QuadtreeValue* value)
	{
	    Add(root_.get(), 0, box_, value);
	}

	void Remove(QuadtreeValue* value)
	{
	    Remove(root_.get(), box_, value);
	}

	std::vector<QuadtreeValue*> Query(Rect box) const
	{
		std::vector<QuadtreeValue*> values;
        Query(root_.get(), box_, box, values);
		return values;
	}

private:
	struct Node
    {
        std::array<std::unique_ptr<Node>, 4> children_;
        std::vector<QuadtreeValue*> values;
    };

	bool IsLeaf(const Node* node) const
    {
        return !static_cast<bool>(node->children_[0]);
    }

    Rect ComputeBox(const Rect& box, int i) const
    {
        const auto& origin = box.min_;
        const auto childSize = box.HalfSize();

        switch (i)
        {
            // North West
            case 0:
                return RectFromOriginAndSize(Vector2F(origin.x_, origin.y_ + childSize.y_), childSize);
            // Norst East
            case 1:
                return RectFromOriginAndSize(origin + childSize, childSize);
            // South West
            case 2:
                return RectFromOriginAndSize(origin, childSize);
            // South East
            case 3:
                return RectFromOriginAndSize(Vector2F(origin.x_ + childSize.x_, origin.y_), childSize);
            default:
                assert(false && "Invalid child index");
                return {};
        }
    }

	int GetQuadrant(const Rect& nodeBox, const Rect& valueBox) const
    {
        const auto center = nodeBox.Center();
        // West
        if (valueBox.max_.x_ < center.x_)
        {
            // North West
            if (valueBox.min_.y_ > center.y_)
                return 0;
            // South West
            else if (valueBox.max_.y_ <= center.y_)
                return 2;
            // Not contained in any quadrant
            else
                return -1;
        }
        // East
        else if (valueBox.min_.x_ >= center.x_)
        {
            // North East
            if (valueBox.min_.y_ > center.y_)
                return 1;
            // South East
            else if (valueBox.max_.y_ <= center.y_)
                return 3;
            // Not contained in any quadrant
            else
                return -1;
        }
        // Not contained in any quadrant
        else
            return -1;
    }

	void Add(Node* node, std::size_t depth, const Rect& box, QuadtreeValue* value)
	{
        if (IsLeaf(node))
        {
            // Insert the value in this node if possible
            if (depth >= QUADTREE_MAX_DEPTH || node->values.size() < QUADTREE_THRESHOLD)
                node->values.push_back(value);
            // Otherwise, we split and we try again
            else
            {
                Split(node, box);
                Add(node, depth, box, value);
            }
        }
        else
        {
            const auto i = GetQuadrant(box, value->box_);
            // Add the value in a child if the value is entirely contained in it
            if (i != -1)
                Add(node->children_[static_cast<std::size_t>(i)].get(), depth + 1, ComputeBox(box, i), value);
            // Otherwise, we add the value in the current node
            else
                node->values.push_back(value);
        }
	}

    void Split(Node* node, const Rect& box)
    {
        // Create children
        for (auto& child : node->children_)
            child = std::make_unique<Node>();

        // Assign values to children
        auto newValues = std::vector<QuadtreeValue*>(); // New values for this node
        for (const auto& value : node->values)
        {
            const auto i = GetQuadrant(box, value->box_);
            if (i != -1)
                node->children_[static_cast<std::size_t>(i)]->values.push_back(value);
            else
                newValues.push_back(value);
        }

        node->values = std::move(newValues);
    }

    bool Remove(Node* node, const Rect& box, QuadtreeValue* value)
    {
        if (IsLeaf(node))
        {
            // Remove the value from node
            RemoveValue(node, value);
            return true;
        }
        else
        {
            // Remove the value in a child if the value is entirely contained in it
            auto i = GetQuadrant(box, value->box_);
            if (i != -1)
            {
                if (Remove(node->children_[static_cast<std::size_t>(i)].get(), ComputeBox(box, i), value))
                    return TryMerge(node);
            }
            // Otherwise, we remove the value from the current node
            else
                RemoveValue(node, value);

            return false;
        }
    }

    void RemoveValue(Node* node, const QuadtreeValue* value)
    {
        // Find the value in node->values
        auto it = std::find_if(std::begin(node->values), std::end(node->values),
            [&value](QuadtreeValue* rhs){ return rhs == value; });

        assert(it != std::end(node->values) && "Trying to remove a value that is not present in the node");

        // Swap with the last element and pop back
        *it = std::move(node->values.back());
        node->values.pop_back();
    }

    bool TryMerge(Node* node)
    {
        auto nbValues = node->values.size();
        for (const auto& child : node->children_)
        {
            if (!IsLeaf(child.get()))
                return false;

            nbValues += child->values.size();
        }

        if (nbValues <= QUADTREE_THRESHOLD)
        {
            node->values.reserve(nbValues);

            // Merge the values of all the children
            for (const auto& child : node->children_)
            {
                for (const auto& value : child->values)
                    node->values.push_back(value);
            }

            // Remove the children
            for (auto& child : node->children_)
                child.reset();

            return true;
        }
        else
            return false;
    }

    void Query(Node* node, const Rect& box, const Rect& queryBox, std::vector<QuadtreeValue*>& values) const
    {
        for (const auto& value : node->values)
        {
            if (queryBox.IsInside(value->box_) != Intersection::OUTSIDE)
                values.push_back(value);
        }

        if (!IsLeaf(node))
        {
            for (std::size_t i = 0; i < node->children_.size(); ++i)
            {
                const auto childBox = ComputeBox(box, static_cast<int>(i));
                if (queryBox.IsInside(childBox) != Intersection::OUTSIDE)
                    Query(node->children_[i].get(), childBox, queryBox, values);
            }
        }
    }

	Rect box_;

	std::unique_ptr<Node> root_;
};

}