using System;
using System.Reflection;

namespace SettingsCompiler
{
    public struct Direction
    {
        public float X;
        public float Y;
        public float Z;

        public Direction(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }
    }

    public struct Color
    {
        public float R;
        public float G;
        public float B;

        public Color(float r, float g, float b)
        {
            R = r;
            G = g;
            B = b;
        }
    }

    public struct Orientation
    {
        public float X;
        public float Y;
        public float Z;
        public float W;

        public static Orientation Identity = new Orientation(0.0f, 0.0f, 0.0f, 1.0f);

        public Orientation(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }
    }

    [AttributeUsage(AttributeTargets.Field, Inherited = false, AllowMultiple = false)]
    public class DisplayNameAttribute : Attribute
    {
        public readonly string DisplayName;

        public DisplayNameAttribute(string displayName)
        {
            this.DisplayName = displayName;
        }

        public static void GetDisplayName(FieldInfo field, ref string displayName)
        {
            DisplayNameAttribute attr = field.GetCustomAttribute<DisplayNameAttribute>();
            if(attr != null)
                displayName = attr.DisplayName;
        }
    }

    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Class, Inherited = false, AllowMultiple = false)]
    public class HelpTextAttribute : Attribute
    {
        public readonly string HelpText;

        public HelpTextAttribute(string helpText)
        {
            this.HelpText = helpText;
        }

        public static void GetHelpText(FieldInfo field, ref string helpText)
        {
            HelpTextAttribute attr = field.GetCustomAttribute<HelpTextAttribute>();
            if(attr != null)
                helpText = attr.HelpText;
        }
    }

    [AttributeUsage(AttributeTargets.Field, Inherited = false, AllowMultiple = false)]
    public class GroupAttribute : Attribute
    {
        public readonly string Group;

        public GroupAttribute(string group)
        {
            this.Group = group;
        }

        public static void GetGroup(FieldInfo field, ref string group)
        {
            GroupAttribute attr = field.GetCustomAttribute<GroupAttribute>();
            if(attr != null)
                group = attr.Group;
        }
    }

    [AttributeUsage(AttributeTargets.Field, Inherited = false, AllowMultiple = false)]
    public class MinValueAttribute : Attribute
    {
        public readonly float MinValueFloat;
        public readonly int MinValueInt;

        public MinValueAttribute(float minValue)
        {
            this.MinValueFloat = minValue;
            this.MinValueInt = (int)minValue;
        }

        public MinValueAttribute(int minValue)
        {
            this.MinValueFloat = (float)minValue;
            this.MinValueInt = minValue;
        }

        public static void GetMinValue(FieldInfo field, ref float minValue)
        {
            MinValueAttribute attr = field.GetCustomAttribute<MinValueAttribute>();
            if(attr != null)
                minValue = attr.MinValueFloat;
        }

        public static void GetMinValue(FieldInfo field, ref int minValue)
        {
            MinValueAttribute attr = field.GetCustomAttribute<MinValueAttribute>();
            if(attr != null)
                minValue = attr.MinValueInt;
        }
    }

    [AttributeUsage(AttributeTargets.Field, Inherited = false, AllowMultiple = false)]
    public class MaxValueAttribute : Attribute
    {
        public readonly float MaxValueFloat;
        public readonly int MaxValueInt;

        public MaxValueAttribute(float maxValue)
        {
            this.MaxValueFloat = maxValue;
            this.MaxValueInt = (int)maxValue;
        }

        public MaxValueAttribute(int maxValue)
        {
            this.MaxValueFloat = (float)maxValue;
            this.MaxValueInt = maxValue;
        }

        public static void GetMaxValue(FieldInfo field, ref float maxValue)
        {
            MaxValueAttribute attr = field.GetCustomAttribute<MaxValueAttribute>();
            if(attr != null)
                maxValue = attr.MaxValueFloat;
        }

        public static void GetMaxValue(FieldInfo field, ref int maxValue)
        {
            MaxValueAttribute attr = field.GetCustomAttribute<MaxValueAttribute>();
            if(attr != null)
                maxValue = attr.MaxValueInt;
        }
    }

    [AttributeUsage(AttributeTargets.Field, Inherited = false, AllowMultiple = false)]
    public class StepSizeAttribute : System.Attribute
    {
        public readonly float StepSizeFloat;
        public readonly int StepSizeInt;

        public StepSizeAttribute(float stepSize)
        {
            this.StepSizeFloat = stepSize;
            this.StepSizeInt = (int)stepSize;
        }

        public StepSizeAttribute(int stepSize)
        {
            this.StepSizeFloat = (float)stepSize;
            this.StepSizeInt = stepSize;
        }

        public static void GetStepSize(FieldInfo field, ref float stepSize)
        {
            StepSizeAttribute attr = field.GetCustomAttribute<StepSizeAttribute>();
            if(attr != null)
                stepSize = attr.StepSizeFloat;
        }

        public static void GetStepSize(FieldInfo field, ref int stepSize)
        {
            StepSizeAttribute attr = field.GetCustomAttribute<StepSizeAttribute>();
            if(attr != null)
                stepSize = attr.StepSizeInt;
        }
    }

    [AttributeUsage(AttributeTargets.Field, Inherited = false, AllowMultiple = false)]
    public class EnumLabelAttribute : System.Attribute
    {
        public readonly string Label;
        public EnumLabelAttribute(string label)
        {
            Label = label;
        }
    }

    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Class, Inherited = false, AllowMultiple = false)]
    public class UseAsShaderConstantAttribute : Attribute
    {
        public readonly bool UseAsShaderConstant = true;

        public UseAsShaderConstantAttribute(bool useAsShaderConstant)
        {
            this.UseAsShaderConstant = useAsShaderConstant;
        }

        public static bool UseFieldAsShaderConstant(FieldInfo field)
        {
            UseAsShaderConstantAttribute attr = field.GetCustomAttribute<UseAsShaderConstantAttribute>();
            if(attr != null)
                return attr.UseAsShaderConstant;
            else
                return true;
        }
    }

    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Class, Inherited = false, AllowMultiple = false)]
    public class HDRAttribute : Attribute
    {
        public readonly bool HDR = false;

        public HDRAttribute(bool hdr)
        {
            this.HDR = hdr;
        }

        public static bool HDRColor(FieldInfo field)
        {
            HDRAttribute attr = field.GetCustomAttribute<HDRAttribute>();
            if(attr != null)
                return attr.HDR;
            else
                return false;
        }
    }
}